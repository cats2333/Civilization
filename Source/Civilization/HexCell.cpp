#include "HexCell.h"
#include "HexMetrics.h"
#include "HexDirection.h"

#define LOG_TO_FILE(Category, Verbosity, Format, ...) \
    UE_LOG(Category, Verbosity, Format, ##__VA_ARGS__); \
    GLog->Logf(ELogVerbosity::Verbosity, TEXT("%s"), *FString::Printf(Format, ##__VA_ARGS__));

AHexCell::AHexCell()
{
    PrimaryActorTick.bCanEverTick = false;

    Neighbors.Init(nullptr, 6);
    PerturbedCorners.SetNum(6); // Initialize empty, will be set during triangulation

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    Color = FLinearColor::White;
    bHasOutgoingRoad = false;
    bHasIncomingRoad = false;
    bIsHighlighted = false;
    Elevation = 0;

    HighlightMeshComponent = nullptr;
    HighlightMaterial = nullptr;
}

void AHexCell::SetupCell(int32 X, int32 Z, const FVector& Position)
{
    SetActorLocation(Position);

    Coordinates = FHexCoordinates::FromOffsetCoordinates(X, Z);
    Coordinates.Y = -Coordinates.X - Coordinates.Z;
    UE_LOG(LogTemp, Log, TEXT("Cell at position (%d, %d) has coordinates (%d, %d, %d)"), X, Z, Coordinates.X, Coordinates.Y, Coordinates.Z);
}

AHexCell* AHexCell::GetNeighbor(EHexDirection Direction) const
{
    return Neighbors[static_cast<int32>(Direction)];
}

void AHexCell::SetNeighbor(EHexDirection Direction, AHexCell* Cell)
{
    Neighbors[static_cast<int32>(Direction)] = Cell;
    if (Cell)
    {
        Cell->Neighbors[static_cast<int32>(HexMetrics::Opposite(Direction))] = this;
    }
}

HexMetrics::EHexEdgeType AHexCell::GetEdgeType(EHexDirection Direction)
{
    AHexCell* Neighbor = GetNeighbor(Direction);
    if (!Neighbor)
    {
        return HexMetrics::EHexEdgeType::Cliff;
    }
    return HexMetrics::GetEdgeType(Elevation, Neighbor->Elevation);
}

HexMetrics::EHexEdgeType AHexCell::GetEdgeType(AHexCell* OtherCell)
{
    if (!OtherCell)
    {
        return HexMetrics::EHexEdgeType::Cliff;
    }
    return HexMetrics::GetEdgeType(Elevation, OtherCell->Elevation);
}

void AHexCell::SetElevation(int32 NewElevation)
{
    Elevation = NewElevation;
    FVector Position = GetActorLocation();
    Position.Z = NewElevation * HexMetrics::ElevationStep;
    SetActorLocation(Position);

    if (Chunk)
    {
        Chunk->Refresh();
    }
}

void AHexCell::RemoveOutgoingRoad()
{
    if (!bHasOutgoingRoad)
    {
        return;
    }

    bHasOutgoingRoad = false;
    RefreshSelfOnly();

    AHexCell* Neighbor = GetNeighbor(OutgoingRoad);
    if (Neighbor)
    {
        Neighbor->bHasIncomingRoad = false;
        Neighbor->RefreshSelfOnly();
    }
}

void AHexCell::RemoveIncomingRoad()
{
    if (!bHasIncomingRoad)
    {
        return;
    }

    bHasIncomingRoad = false;
    RefreshSelfOnly();

    AHexCell* Neighbor = GetNeighbor(IncomingRoad);
    if (Neighbor)
    {
        Neighbor->bHasOutgoingRoad = false;
        Neighbor->RefreshSelfOnly();
    }
}

void AHexCell::RemoveRoad()
{
    RemoveOutgoingRoad();
    RemoveIncomingRoad();
}

void AHexCell::SetOutgoingRoad(EHexDirection Direction)
{
    if (bHasOutgoingRoad && OutgoingRoad == Direction)
    {
        return;
    }

    AHexCell* Neighbor = GetNeighbor(Direction);
    if (!Neighbor)
    {
        return;
    }

    RemoveOutgoingRoad();
    if (bHasIncomingRoad && IncomingRoad == Direction)
    {
        RemoveIncomingRoad();
    }

    bHasOutgoingRoad = true;
    OutgoingRoad = Direction;
    UE_LOG(LogTemp, Log, TEXT("SetOutgoingRoad: Cell (%d, %d) -> Dir %d"), Coordinates.X, Coordinates.Z, static_cast<int32>(Direction));
    RefreshSelfOnly();

    Neighbor->RemoveIncomingRoad();
    Neighbor->bHasIncomingRoad = true;
    Neighbor->IncomingRoad = HexMetrics::Opposite(Direction);
    UE_LOG(LogTemp, Log, TEXT("SetIncomingRoad: Neighbor (%d, %d) <- Dir %d"), Neighbor->Coordinates.X, Neighbor->Coordinates.Z, static_cast<int32>(Neighbor->IncomingRoad));
    Neighbor->RefreshSelfOnly();
}

void AHexCell::SetIncomingRoad(EHexDirection Direction)
{
    if (bHasIncomingRoad && IncomingRoad == Direction)
    {
        return;
    }

    AHexCell* Neighbor = GetNeighbor(Direction);
    if (!Neighbor)
    {
        return;
    }

    RemoveIncomingRoad();
    if (bHasOutgoingRoad && OutgoingRoad == Direction)
    {
        RemoveOutgoingRoad();
    }

    bHasIncomingRoad = true;
    IncomingRoad = Direction;
    UE_LOG(LogTemp, Log, TEXT("SetIncomingRoad: Cell (%d, %d) <- Dir %d"), Coordinates.X, Coordinates.Z, static_cast<int32>(Direction));
    RefreshSelfOnly();
}

bool AHexCell::HasRoad() const
{
    bool Result = bHasIncomingRoad || bHasOutgoingRoad;
    return Result;
}

bool AHexCell::HasRoadThroughEdge(EHexDirection Direction) const
{
    bool Result = (bHasIncomingRoad && IncomingRoad == Direction) ||
        (bHasOutgoingRoad && OutgoingRoad == Direction);
    return Result;
}

void AHexCell::RefreshSelfOnly()
{
    if (Chunk)
    {
        Chunk->Refresh();
    }
}

void AHexCell::SetHighlighted(bool bHighlight, FVector CenterWorldPosition)
{
    if (bIsHighlighted == bHighlight)
    {
        return;
    }

    bIsHighlighted = bHighlight;

    if (bIsHighlighted)
    {
        if (!HighlightMeshComponent)
        {
            HighlightMeshComponent = NewObject<UProceduralMeshComponent>(this, TEXT("HighlightMesh"));
            HighlightMeshComponent->RegisterComponent();
            HighlightMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

            UMaterialInterface* MaterialToUse = HighlightMaterial;
            if (!MaterialToUse)
            {
                MaterialToUse = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultDecalMaterial"));
            }

            UMaterialInstanceDynamic* DynamicMaterial = nullptr;
            if (UMaterialInstanceDynamic* ExistingDynamicMaterial = Cast<UMaterialInstanceDynamic>(MaterialToUse))
            {
                DynamicMaterial = ExistingDynamicMaterial;
            }
            else
            {
                DynamicMaterial = UMaterialInstanceDynamic::Create(MaterialToUse, this);
            }
            if (DynamicMaterial)
            {
                DynamicMaterial->TwoSided = true;
                HighlightMeshComponent->SetMaterial(0, DynamicMaterial);
            }
            else
            {
                HighlightMeshComponent->SetMaterial(0, MaterialToUse);
            }

            HighlightMeshComponent->bUseComplexAsSimpleCollision = true;
            HighlightMeshComponent->SetRenderCustomDepth(true);
            HighlightMeshComponent->CustomDepthStencilValue = 1;
        }

        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FColor> VertexColors;
        TArray<FVector2D> UV0;
        TArray<FProcMeshTangent> Tangents;

        FVector Center = CenterWorldPosition;

        float HighlightOffsetZ = 0.0f;
        float OutlineWidth = HexMetrics::OuterRadius * 0.1f;

        const float SpacingFactor = HexMetrics::SpacingFactor;
        int32 X = Coordinates.X + (Coordinates.Z - (Coordinates.Z & 1)) / 2;
        int32 Z = Coordinates.Z;
        float PosX = (X + Z * 0.5f - Z / 2) * (HexMetrics::InnerRadius * 2.0f) * SpacingFactor;
        float PosY = Z * (HexMetrics::OuterRadius * 1.5f) * SpacingFactor;
        FVector AdjustedCenter = Center + FVector(PosX, PosY, 0.0f);

        UE_LOG(LogTemp, Log, TEXT("Coordinates = (%d, %d, %d), WorldPosition = %s, Center = %s, AdjustedCenter = %s"),
            Coordinates.X, Coordinates.Y, Coordinates.Z,
            *CenterWorldPosition.ToString(),
            *Center.ToString(),
            *AdjustedCenter.ToString());

        TArray<FVector> Corners;
        Corners.SetNum(6);
        for (int32 i = 0; i < 6; i++)
        {
            if (PerturbedCorners[i].IsZero())
            {
                UE_LOG(LogTemp, Warning, TEXT("PerturbedCorners not initialized for cell (%d, %d), generating now"),
                    Coordinates.X, Coordinates.Z);
                FVector BaseCorner = HexMetrics::Corners[i];
                FVector WorldCorner = GetActorLocation() + BaseCorner;
                FVector PerturbedWorldCorner = HexMetrics::Perturb(WorldCorner);
                Corners[i] = PerturbedWorldCorner - GetActorLocation();
            }
            else
            {
                Corners[i] = PerturbedCorners[i];
            }
            // Apply SolidFactor to match the mesh's inner solid part
            Corners[i] *= HexMetrics::SolidFactor;
        }

        for (int32 i = 0; i < 6; i++)
        {
            Corners[i] = AdjustedCenter + Corners[i];
            UE_LOG(LogTemp, Log, TEXT("Highlight Corner %d for cell (%d, %d): %s"),
                i, Coordinates.X, Coordinates.Z, *Corners[i].ToString());
        }

        for (int32 i = 0; i < 6; i++)
        {
            FVector OuterCorner = Corners[i];
            FVector DirectionToCenter = (AdjustedCenter - OuterCorner).GetSafeNormal();
            FVector InnerCorner = OuterCorner + DirectionToCenter * OutlineWidth;

            int32 NextI = (i + 1) % 6;
            FVector NextOuterCorner = Corners[NextI];
            FVector NextDirectionToCenter = (AdjustedCenter - NextOuterCorner).GetSafeNormal();
            FVector NextInnerCorner = NextOuterCorner + NextDirectionToCenter * OutlineWidth;

            OuterCorner.Z = HighlightOffsetZ;
            InnerCorner.Z = HighlightOffsetZ;
            NextOuterCorner.Z = HighlightOffsetZ;
            NextInnerCorner.Z = HighlightOffsetZ;

            int32 VertexIndex = Vertices.Num();
            Vertices.Add(OuterCorner);
            Vertices.Add(InnerCorner);
            Vertices.Add(NextOuterCorner);
            Vertices.Add(NextInnerCorner);

            Normals.Add(FVector(0, 0, 1));
            Normals.Add(FVector(0, 0, 1));
            Normals.Add(FVector(0, 0, 1));
            Normals.Add(FVector(0, 0, 1));

            VertexColors.Add(FColor::Yellow);
            VertexColors.Add(FColor::Yellow);
            VertexColors.Add(FColor::Yellow);
            VertexColors.Add(FColor::Yellow);

            Triangles.Add(VertexIndex);
            Triangles.Add(VertexIndex + 1);
            Triangles.Add(VertexIndex + 2);
            Triangles.Add(VertexIndex + 1);
            Triangles.Add(VertexIndex + 3);
            Triangles.Add(VertexIndex + 2);
        }

        UV0.Init(FVector2D(0, 0), Vertices.Num());
        Tangents.Init(FProcMeshTangent(1, 0, 0), Vertices.Num());

        HighlightMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
        HighlightMeshComponent->SetVisibility(true);

        HighlightMeshComponent->SetWorldLocation(CenterWorldPosition);

        UE_LOG(LogTemp, Log, TEXT("HighlightMeshComponent Visibility = %d, IsRegistered = %d, World Position = %s"),
            HighlightMeshComponent->IsVisible(),
            HighlightMeshComponent->IsRegistered(),
            *HighlightMeshComponent->GetComponentLocation().ToString());
    }
    else
    {
        if (HighlightMeshComponent)
        {
            HighlightMeshComponent->ClearAllMeshSections();
            HighlightMeshComponent->SetVisibility(false);
            HighlightMeshComponent->DestroyComponent();
            HighlightMeshComponent = nullptr;
        }

        UE_LOG(LogTemp, Log, TEXT("Removed highlight from cell (%d, %d)"), Coordinates.X, Coordinates.Z);
    }
}

void AHexCell::Perturb()
{
    PerturbedCorners.Empty();
    PerturbedCorners.SetNum(6);

    // Get the world position of the cell to align perturbation with world space
    FVector CellWorldPosition = GetActorLocation();

    for (int32 i = 0; i < 6; i++)
    {
        FVector BaseCorner = HexMetrics::Corners[i];
        // Transform the local corner position to world space before perturbing
        FVector WorldCorner = CellWorldPosition + BaseCorner;
        FVector PerturbedWorldCorner = HexMetrics::Perturb(WorldCorner);
        // Transform back to local space relative to the cell
        PerturbedCorners[i] = PerturbedWorldCorner - CellWorldPosition;
        UE_LOG(LogTemp, Log, TEXT("Perturbed Corner %d for cell (%d, %d): BaseCorner=%s, WorldCorner=%s, PerturbedWorldCorner=%s, PerturbedCorners[%d]=%s"),
            i, Coordinates.X, Coordinates.Z,
            *BaseCorner.ToString(), *WorldCorner.ToString(), *PerturbedWorldCorner.ToString(),
            i, *PerturbedCorners[i].ToString());
    }
}