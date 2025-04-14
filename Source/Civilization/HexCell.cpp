#include "HexCell.h"
#include "HexMetrics.h"
#include "HexDirection.h"

AHexCell::AHexCell()
{
    PrimaryActorTick.bCanEverTick = false;

    Neighbors.Init(nullptr, 6);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    Color = FLinearColor::White;
    bHasOutgoingRoad = false;
    bHasIncomingRoad = false;
    bIsHighlighted = false;
    Elevation = 0;
}

void AHexCell::SetHighlight(bool bHighlight)
{
    if (bIsHighlighted == bHighlight)
    {
        return;
    }

    bIsHighlighted = bHighlight;
    UE_LOG(LogTemp, Log, TEXT("Cell (%d, %d): Highlight %s"),
        Coordinates.X, Coordinates.Z, bHighlight ? TEXT("ON") : TEXT("OFF"));

    if (Chunk)
    {
        Chunk->SetCellHighlight(this, bHighlight);
    }
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