#include "HexMapEditor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"

#define LOG_TO_FILE(Category, Verbosity, Format, ...) \
    UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__); \
    GLog->Logf(ELogVerbosity::Verbosity, TEXT("%s"), *FString::Printf(Format, ##__VA_ARGS__));

UHexMapEditor::UHexMapEditor()
{
    PrimaryComponentTick.bCanEverTick = true;
    bIsFirstClick = true;
    PreviousCell = nullptr;
    BrushSize = 2;
    ActiveElevation = 0;
    MoveSpeed = 50.0f;
    SwivelMinZoom = 5.0f;
    SwivelMaxZoom = 80.0f;
}

void UHexMapEditor::BeginPlay()
{
    Super::BeginPlay();
    bIsFirstClick = true;
    PreviousCell = nullptr;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        if (APawn* PlayerPawn = PC->GetPawn())
        {
            LOG_TO_FILE(LogTemp, Log, TEXT("Pawn Class: %s"), *PlayerPawn->GetClass()->GetName());
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
                LOG_TO_FILE(LogTemp, Log, TEXT("HexGrid is set! CellClass: %s, ChunkClass: %s"),
                    *GetNameSafe(HexGrid->CellClass), *GetNameSafe(HexGrid->ChunkClass));
            }
            else
            {
                LOG_TO_FILE(LogTemp, Warning, TEXT("HexGrid is null in BeginPlay! Please set it in the editor."));
            }

            UCameraComponent* Camera = PlayerPawn->FindComponentByClass<UCameraComponent>();
            if (!Camera)
            {
                Camera = NewObject<UCameraComponent>(PlayerPawn, TEXT("CameraComponent"));
                Camera->SetupAttachment(PlayerPawn->GetRootComponent());
                Camera->SetRelativeLocation(FVector(0.0f, 0.0f, 1000.0f));
                Camera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
                Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
                Camera->OrthoWidth = 50.0f;
                Camera->RegisterComponent();
                LOG_TO_FILE(LogTemp, Log, TEXT("Created Orthographic Camera, OrthoWidth=%f"), Camera->OrthoWidth);
            }
            else
            {
                Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
                Camera->SetRelativeLocation(FVector(0.0f, 0.0f, 1000.0f));
                Camera->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
                Camera->OrthoWidth = 50.0f;
                LOG_TO_FILE(LogTemp, Log, TEXT("Updated Camera to Orthographic, OrthoWidth=%f"), Camera->OrthoWidth);
            }
        }

        if (UInputComponent* InputComponent = GetOwner()->FindComponentByClass<UInputComponent>())
        {
            InputComponent->BindAxis("MoveForward", this, &UHexMapEditor::MoveCameraForward);
            InputComponent->BindAxis("MoveRight", this, &UHexMapEditor::MoveCameraRight);
            InputComponent->BindAxis("MouseScroll", this, &UHexMapEditor::AdjustCameraZoom);
            LOG_TO_FILE(LogTemp, Log, TEXT("Input bindings set: MoveForward, MoveRight, MouseScroll"));
        }
    }

    SelectColor(0);
    SetBrushSize(2);
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
    // ¿ÕÊµÏÖ£¬´ý UI Âß¼­
}

void UHexMapEditor::HandleInput()
{
    if (!HexGrid)
    {
        UE_LOG(LogTemp, Warning, TEXT("HexGrid is null in HexMapEditor!"));
        return;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("PlayerController is null!"));
        return;
    }

    FVector WorldPos, WorldDir;
    if (!PC->DeprojectMousePositionToWorld(WorldPos, WorldDir))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to deproject mouse position!"));
        return;
    }

    FHitResult HitResult;
    if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldPos, WorldPos + WorldDir * 10000.f, ECC_Visibility))
    {
        AHexCell* CurrentCell = HexGrid->GetCellByPosition(HitResult.Location);
        UE_LOG(LogTemp, Log, TEXT("HandleInput: Hit at (%f, %f, %f), Cell=%s"),
            HitResult.Location.X, HitResult.Location.Y, HitResult.Location.Z,
            CurrentCell ? *GetNameSafe(CurrentCell) : TEXT("nullptr"));
        if (CurrentCell)
        {
            EditCells(CurrentCell);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No cell found at hit position!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Line trace failed!"));
    }
}

void UHexMapEditor::EditCells(AHexCell* Center)
{
    if (!Center || !HexGrid)
    {
        UE_LOG(LogTemp, Warning, TEXT("EditCells: Center=%s, HexGrid=%s"),
            Center ? *GetNameSafe(Center) : TEXT("nullptr"),
            HexGrid ? *GetNameSafe(HexGrid) : TEXT("nullptr"));
        return;
    }

    if (!Center->Chunk)
    {
        UE_LOG(LogTemp, Warning, TEXT("EditCells: Center cell has no Chunk assigned!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("EditCells: Center at (%d, %d), BrushSize=%d"),
        Center->Coordinates.X, Center->Coordinates.Z, BrushSize);

    if (BrushSize <= 1 || EditMode == EEditMode::Road)
    {
        EditCell(Center);
    }
    else
    {
        TArray<AHexCell*> CellsToEdit;
        CellsToEdit.Add(Center);
        for (int32 Ring = 1; Ring <= BrushSize; Ring++)
        {
            for (int32 Dir = 0; Dir < 6; Dir++)
            {
                EHexDirection Direction = static_cast<EHexDirection>(Dir);
                AHexCell* Neighbor = Center;
                for (int32 Step = 0; Step < Ring; Step++)
                {
                    if (Neighbor)
                    {
                        Neighbor = Neighbor->GetNeighbor(Direction);
                    }
                }
                if (Neighbor)
                {
                    CellsToEdit.AddUnique(Neighbor);
                    for (int32 NeighborDir = 0; NeighborDir < 6; NeighborDir++)
                    {
                        AHexCell* SubNeighbor = Neighbor->GetNeighbor(static_cast<EHexDirection>(NeighborDir));
                        if (SubNeighbor)
                        {
                            CellsToEdit.AddUnique(SubNeighbor);
                        }
                    }
                }
            }
        }

        for (AHexCell* Cell : CellsToEdit)
        {
            EditCell(Cell);
        }
    }

    if (EditMode != EEditMode::Road || RoadMode == EEditRoadMode::No)
    {
        HexGrid->Refresh();
    }
}

void UHexMapEditor::EditCell(AHexCell* Cell)
{
    if (!Cell)
    {
        UE_LOG(LogTemp, Warning, TEXT("EditCell: Cell is null!"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("EditCell: Processing cell (%d, %d), Mode=%s, RoadMode=%s, Highlight=%s"),
        Cell->Coordinates.X, Cell->Coordinates.Z,
        *UEnum::GetValueAsString(EditMode),
        *UEnum::GetValueAsString(RoadMode),
        Cell->bIsHighlighted ? TEXT("ON") : TEXT("OFF"));

    switch (EditMode)
    {
    case EEditMode::Color:
        Cell->Color = ActiveColor;
        UE_LOG(LogTemp, Log, TEXT("Set cell (%d, %d) color to %s"),
            Cell->Coordinates.X, Cell->Coordinates.Z, *ActiveColor.ToString());
        if (Cell->Chunk)
        {
            Cell->Chunk->Refresh();
        }
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
            ClearHighlight();
            break;
        case EEditRoadMode::Yes:
            UE_LOG(LogTemp, Log, TEXT("Road mode: bIsFirstClick=%s, PreviousCell=%s"),
                bIsFirstClick ? TEXT("true") : TEXT("false"),
                PreviousCell ? *GetNameSafe(PreviousCell) : TEXT("nullptr"));
            if (bIsFirstClick)
            {
                if (PreviousCell && PreviousCell != Cell)
                {
                    PreviousCell->SetHighlight(false);
                    UE_LOG(LogTemp, Log, TEXT("Clearing previous highlight for cell (%d, %d)"),
                        PreviousCell->Coordinates.X, PreviousCell->Coordinates.Z);
                }
                PreviousCell = Cell;
                bIsFirstClick = false;
                Cell->SetHighlight(true);
                UE_LOG(LogTemp, Log, TEXT("First click on cell (%d, %d), waiting for second click"),
                    Cell->Coordinates.X, Cell->Coordinates.Z);
            }
            else
            {
                if (!PreviousCell)
                {
                    UE_LOG(LogTemp, Warning, TEXT("PreviousCell is null on second click, resetting"));
                    PreviousCell = Cell;
                    bIsFirstClick = false;
                    Cell->SetHighlight(true);
                    UE_LOG(LogTemp, Log, TEXT("Reset to first click on cell (%d, %d)"),
                        Cell->Coordinates.X, Cell->Coordinates.Z);
                    break;
                }

                if (PreviousCell == Cell)
                {
                    UE_LOG(LogTemp, Log, TEXT("Same cell clicked again, clearing highlight"));
                    Cell->SetHighlight(false);
                    PreviousCell = nullptr;
                    bIsFirstClick = true;
                    break;
                }

                bool bIsNeighbor = false;
                EHexDirection NeighborDirection = EHexDirection::E;
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

                PreviousCell->SetHighlight(false);
                UE_LOG(LogTemp, Log, TEXT("Clearing previous highlight for cell (%d, %d)"),
                    PreviousCell->Coordinates.X, PreviousCell->Coordinates.Z);

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
                    UE_LOG(LogTemp, Warning, TEXT("Second click on (%d, %d) is not a neighbor of (%d, %d), please select a neighboring cell"),
                        Cell->Coordinates.X, Cell->Coordinates.Z,
                        PreviousCell->Coordinates.X, PreviousCell->Coordinates.Z);
                }

                PreviousCell = Cell;
                bIsFirstClick = false;
                Cell->SetHighlight(true);
                UE_LOG(LogTemp, Log, TEXT("Second click, set highlight for cell (%d, %d)"),
                    Cell->Coordinates.X, Cell->Coordinates.Z);
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
        UE_LOG(LogTemp, Log, TEXT("Cleared highlight for cell (%d, %d)"),
            PreviousCell->Coordinates.X, PreviousCell->Coordinates.Z);
        PreviousCell = nullptr;
        bIsFirstClick = true;
    }
    if (HexGrid)
    {
        for (AHexCell* Cell : HexGrid->GetCells())
        {
            if (Cell && Cell->bIsHighlighted)
            {
                Cell->SetHighlight(false);
            }
        }
    }
}

void UHexMapEditor::SelectColor(int32 Index)
{
    if (Colors.IsValidIndex(Index))
    {
        ActiveColor = Colors[Index];
        UE_LOG(LogTemp, Log, TEXT("Selected color index: %d, Color: %s"), Index, *ActiveColor.ToString());
    }
    ClearHighlight();
}

void UHexMapEditor::SetBrushSize(float Size)
{
    BrushSize = FMath::RoundToInt(Size);
    UE_LOG(LogTemp, Log, TEXT("Set BrushSize to %d"), BrushSize);
    ClearHighlight();
}

void UHexMapEditor::SetElevation(float Elevation)
{
    ActiveElevation = FMath::RoundToInt(Elevation);
    UE_LOG(LogTemp, Log, TEXT("Set ActiveElevation to %d"), ActiveElevation);
    ClearHighlight();
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

void UHexMapEditor::MoveCameraForward(float AxisValue)
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

}

