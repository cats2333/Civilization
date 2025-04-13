#include "HexMapEditor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputCoreTypes.h"
UHexMapEditor::UHexMapEditor()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UHexMapEditor::BeginPlay()
{
    Super::BeginPlay();
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        if (APawn* PlayerPawn = PC->GetPawn())
        {
            UE_LOG(LogTemp, Log, TEXT("Pawn Class: %s"), *PlayerPawn->GetClass()->GetName());
        }

        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(InputMode);

        if (APawn* PlayerPawn = PC->GetPawn())
        {
            if (HexGrid)
            {
                UE_LOG(LogTemp, Log, TEXT("HexGrid is set! CellClass: %s, ChunkClass: %s"),
                    *GetNameSafe(HexGrid->CellClass), *GetNameSafe(HexGrid->ChunkClass));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("HexGrid is null in BeginPlay! Please set it in the editor."));
            }

            UCameraComponent* Camera = PlayerPawn->FindComponentByClass<UCameraComponent>();
            if (!Camera)
            {
                Camera = NewObject<UCameraComponent>(PlayerPawn, TEXT("CameraComponent"));
                Camera->SetupAttachment(PlayerPawn->GetRootComponent());
                Camera->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
                Camera->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
                Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
                Camera->OrthoWidth = 50.0f;
                Camera->SetOrthoFarClipPlane(100000.0f);
                Camera->SetOrthoNearClipPlane(0.00001f);
                Camera->RegisterComponent();
            }
            else
            {
                Camera->SetRelativeLocation(FVector(-300.0f, 0.0f, 200.0f));
                Camera->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
                Camera->SetOrthoFarClipPlane(100000.0f);
            }
            PlayerPawn->GetRootComponent()->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
        }

        if (UInputComponent* InputComponent = PC->InputComponent)
        {
            InputComponent->BindAxis("MoveForward", this, &UHexMapEditor::MoveCameraForward);
            InputComponent->BindAxis("MoveRight", this, &UHexMapEditor::MoveCameraRight);
            InputComponent->BindAxis("MouseScroll", this, &UHexMapEditor::AdjustCameraZoom);
            InputComponent->BindAxis("LookUp", this, &UHexMapEditor::RotateCamera);
        }
    }

    SelectColor(0);

}

void UHexMapEditor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        UE_LOG(LogTemp, Log, TEXT("Left mouse clicked, bIsFirstClick=%s"), bIsFirstClick ? TEXT("true") : TEXT("false"));
        HandleInput();
    }
}

void UHexMapEditor::ShowEditorUI(bool bVisible)
{
}
void UHexMapEditor::HandleInput()
{
    if (!HexGrid)
    {
        UE_LOG(LogTemp, Warning, TEXT("HexGrid is null in HexMapEditor!"));
        return;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;

    FVector WorldPos, WorldDir;
    PC->DeprojectMousePositionToWorld(WorldPos, WorldDir);

    FHitResult HitResult;
    if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldPos, WorldPos + WorldDir * 10000.f, ECC_Visibility))
    {
        AHexCell* CurrentCell = HexGrid->GetCellByPosition(HitResult.Location);
        if (CurrentCell)
        {
            EditCells(CurrentCell);
        }
    }
}

void UHexMapEditor::EditCells(AHexCell* Center)
{
    if (!Center || !HexGrid) return;

    int32 CenterX = Center->Coordinates.X;
    int32 CenterZ = Center->Coordinates.Z;

    for (int32 r = 0, z = CenterZ - BrushSize; z <= CenterZ; z++, r++)
    {
        for (int32 x = CenterX - r; x <= CenterX + BrushSize; x++)
        {
            AHexCell* Cell = HexGrid->GetCellByCoordinates(FHexCoordinates(x, z));
            EditCell(Cell);
        }
    }

    for (int32 r = 0, z = CenterZ + BrushSize; z > CenterZ; z--, r++)
    {
        for (int32 x = CenterX - BrushSize; x <= CenterX + r; x++)
        {
            AHexCell* Cell = HexGrid->GetCellByCoordinates(FHexCoordinates(x, z));
            EditCell(Cell);
        }
    }

    HexGrid->Refresh();

}
void UHexMapEditor::EditCell(AHexCell* Cell)
{
    if (!Cell)
    {
        UE_LOG(LogTemp, Warning, TEXT("EditCell: Cell is null!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("EditCell: Processing cell (%d, %d), Mode=%s, RoadMode=%s"),
        Cell->Coordinates.X, Cell->Coordinates.Z,
        *UEnum::GetValueAsString(EditMode),
        *UEnum::GetValueAsString(RoadMode));

    switch (EditMode)
    {
    case EEditMode::Color:
        Cell->Color = ActiveColor;
        break;
    case EEditMode::Elevation:
        Cell->SetElevation(ActiveElevation);
        break;
    case EEditMode::Road:
        switch (RoadMode)
        {
        case EEditRoadMode::No:
            Cell->RemoveRoad();
            if (Cell->Chunk)
            {
                Cell->Chunk->ClearRoadDecals();
            }
            break;
        case EEditRoadMode::Yes:
            UE_LOG(LogTemp, Log, TEXT("Road mode: bIsFirstClick=%s, PreviousCell=%s"),
                bIsFirstClick ? TEXT("true") : TEXT("false"),
                PreviousCell ? *GetNameSafe(PreviousCell) : TEXT("nullptr"));
            if (bIsFirstClick)
            {
                if (PreviousCell)
                {
                    PreviousCell->SetHighlight(false);
                }
                PreviousCell = Cell;
                bIsFirstClick = false;
                Cell->SetHighlight(true);
                UE_LOG(LogTemp, Log, TEXT("First click on cell (%d, %d), waiting for second click"),
                    Cell->Coordinates.X, Cell->Coordinates.Z);
            }
            else
            {
                if (PreviousCell)
                {
                    PreviousCell->SetHighlight(false);
                }

                bool bIsNeighbor = false;
                EHexDirection NeighborDirection = EHexDirection::E;
                if (PreviousCell)
                {
                    UE_LOG(LogTemp, Log, TEXT("Second click, checking neighbors"));
                    for (int32 i = 0; i < 6; i++)
                    {
                        EHexDirection Dir = static_cast<EHexDirection>(i);
                        AHexCell* Neighbor = PreviousCell->GetNeighbor(Dir);
                        if (Neighbor == Cell)
                        {
                            bIsNeighbor = true;
                            NeighborDirection = Dir;
                            break;
                        }
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("PreviousCell is null on second click!"));
                }

                if (bIsNeighbor)
                {
                    PreviousCell->SetOutgoingRoad(NeighborDirection);
                    if (PreviousCell->Chunk)
                    {
                        FVector Start = PreviousCell->GetPosition();
                        FVector End = Cell->GetPosition();
                        PreviousCell->Chunk->CreateRoadDecal(Start, End, 0.5f, PreviousCell->Chunk->RoadDecalMaterial);
                        UE_LOG(LogTemp, Log, TEXT("Road created from (%d, %d) to (%d, %d), Direction: %d"),
                            PreviousCell->Coordinates.X, PreviousCell->Coordinates.Z,
                            Cell->Coordinates.X, Cell->Coordinates.Z, static_cast<int32>(NeighborDirection));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Second click on (%d, %d) is not a neighbor of (%d, %d), no road created"),
                        Cell->Coordinates.X, Cell->Coordinates.Z,
                        PreviousCell ? PreviousCell->Coordinates.X : -1,
                        PreviousCell ? PreviousCell->Coordinates.Z : -1);
                }

                PreviousCell = Cell;
                bIsFirstClick = true;
                Cell->SetHighlight(true);
            }
            break;
        case EEditRoadMode::Ignore:
        default:
            ClearHighlight();
            break;
        }
        break;
    }
}

void UHexMapEditor::ClearHighlight()
{
    if (PreviousCell)
    {
        PreviousCell->SetHighlight(false);
        PreviousCell = nullptr;
        bIsFirstClick = true;
        UE_LOG(LogTemp, Log, TEXT("Cleared highlight and reset state"));
    }
}

void UHexMapEditor::SetBrushSize(float Size)
{
    BrushSize = FMath::RoundToInt(Size);
    UE_LOG(LogTemp, Log, TEXT("Set BrushSize to %d"), BrushSize);
}void UHexMapEditor::SetElevation(float Elevation)
{
    ActiveElevation = FMath::RoundToInt(Elevation);
    UE_LOG(LogTemp, Log, TEXT("Set ActiveElevation to %d"), ActiveElevation);
}void UHexMapEditor::SelectColor(int32 Index)
{
    if (Colors.IsValidIndex(Index))
    {
        ActiveColor = Colors[Index];
        UE_LOG(LogTemp, Log, TEXT("Selected color index: %d, Color: %s"), Index, *ActiveColor.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid color index: %d"), Index);
    }
}void UHexMapEditor::MoveCameraForward(float AxisValue)
{
    MoveCamera(AxisValue, true);
}void UHexMapEditor::MoveCameraRight(float AxisValue)
{
    MoveCamera(AxisValue, false);
}void UHexMapEditor::MoveCamera(float AxisValue, bool bIsForward)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->GetPawn() && AxisValue != 0.0f)
    {
        if (UCameraComponent* Camera = PC->GetPawn()->FindComponentByClass<UCameraComponent>())
        {
            FVector PawnLocation = PC->GetPawn()->GetActorLocation();
            float MoveDelta = AxisValue * MoveSpeed * GetWorld()->GetDeltaSeconds();
            FVector Direction = bIsForward ? Camera->GetForwardVector() : Camera->GetRightVector();
            Direction.Z = 0.0f;
            Direction.Normalize();

            PawnLocation += Direction * MoveDelta;
            PC->GetPawn()->SetActorLocation(PawnLocation);
        }
    }

}void UHexMapEditor::AdjustCameraZoom(float Value)
{
    if (FMath::Abs(Value) < 0.05f) return;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->GetPawn())
    {
        if (UCameraComponent* Camera = PC->GetPawn()->FindComponentByClass<UCameraComponent>())
        {
            float NewOrthoWidth = Camera->OrthoWidth - (Value * 10.0f);
            NewOrthoWidth = FMath::Clamp(NewOrthoWidth, 10.0f, 1000.0f);
            Camera->OrthoWidth = NewOrthoWidth;
        }
    }

}void UHexMapEditor::RotateCamera(float AxisValue)
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->GetPawn() && AxisValue != 0.0f && PC->IsInputKeyDown(EKeys::RightMouseButton))
    {
        if (UCameraComponent* Camera = PC->GetPawn()->FindComponentByClass<UCameraComponent>())
        {
            FRotator CameraRotation = Camera->GetRelativeRotation();
            CameraRotation.Pitch = FMath::Clamp(CameraRotation.Pitch + AxisValue * 100.0f * GetWorld()->GetDeltaSeconds(), -SwivelMaxZoom, -SwivelMinZoom);
            Camera->SetRelativeRotation(CameraRotation);
        }
    }
}

TArray<FString> UHexMapEditor::GetEditModeOptions()
{
    TArray<FString> Options;
    UEnum* Enum = StaticEnum<EEditMode>();
    if (Enum)
    {
        for (int32 i = 0; i < Enum->NumEnums() - 1; i++)
        {
            FString EnumValue = Enum->GetNameStringByIndex(i);
            Options.Add(EnumValue);
        }
    }
    return Options;
}

TArray<FString> UHexMapEditor::GetRoadModeOptions()
{
    TArray<FString> Options;
    UEnum* Enum = StaticEnum<EEditRoadMode>();
    if (Enum)
    {
        for (int32 i = 0; i < Enum->NumEnums() - 1; i++)
        {
            FString EnumValue = Enum->GetNameStringByIndex(i);
            Options.Add(EnumValue);
        }
    }
    return Options;
}
