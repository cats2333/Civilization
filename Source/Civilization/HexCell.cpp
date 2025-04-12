#include "HexCell.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/SceneComponent.h"
#include "HexMetrics.h"
#include "HexDirection.h"
AHexCell::AHexCell()
{
    PrimaryActorTick.bCanEverTick = false;

    // 初始化 Neighbors 数组，六个方向
    Neighbors.Init(nullptr, 6);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    Color = FLinearColor::White;

}void AHexCell::SetupCell(int32 X, int32 Z, const FVector& Position)
{
    SetActorLocation(Position);

    Coordinates = FHexCoordinates::FromOffsetCoordinates(X, Z);
    Coordinates.Y = -Coordinates.X - Coordinates.Z;
    UE_LOG(LogTemp, Log, TEXT("Cell at position (%d, %d) has coordinates (%d, %d, %d)"), X, Z, Coordinates.X, Coordinates.Y, Coordinates.Z);

}AHexCell* AHexCell::GetNeighbor(EHexDirection Direction) const
{
    return Neighbors[static_cast<int32>(Direction)];
}void AHexCell::SetNeighbor(EHexDirection Direction, AHexCell* Cell)
{
    Neighbors[static_cast<int32>(Direction)] = Cell;
    if (Cell)
    {
        Cell->Neighbors[static_cast<int32>(HexMetrics::Opposite(Direction))] = this;
    }
}HexMetrics::EHexEdgeType AHexCell::GetEdgeType(EHexDirection Direction)
{
    AHexCell* Neighbor = GetNeighbor(Direction);
    if (!Neighbor)
    {
        return HexMetrics::EHexEdgeType::Cliff; // 没有邻居时，视为悬崖
    }
    return HexMetrics::GetEdgeType(Elevation, Neighbor->Elevation);
}void AHexCell::SetElevation(int32 NewElevation)
{
    Elevation = NewElevation;
    FVector Position = GetActorLocation();
    Position.Z = NewElevation * HexMetrics::ElevationStep;
    //FVector4 NoiseSample = HexMetrics::SampleNoise(Position);
    //Position.Z += (NoiseSample.Z * 2.0f - 1.0f) * 1.5f; // 从 1.5f 改成 0.5f
    SetActorLocation(Position);

    // 检查流出河流
    if (bHasOutgoingRiver)
    {
        AHexCell* OutNeighbor = GetNeighbor(OutgoingRiver);
        if (OutNeighbor && Elevation < OutNeighbor->Elevation)
        {
            RemoveOutgoingRiver();
        }
    }

    // 检查流入河流
    if (bHasIncomingRiver)
    {
        AHexCell* InNeighbor = GetNeighbor(IncomingRiver);
        if (InNeighbor && Elevation > InNeighbor->Elevation)
        {
            RemoveIncomingRiver();
        }
    }

    // 刷新（原有的Refresh调用）
    if (Chunk)
    {
        Chunk->Refresh();
    }

}HexMetrics::EHexEdgeType AHexCell::GetEdgeType(AHexCell* OtherCell)
{
    if (!OtherCell)
    {
        return HexMetrics::EHexEdgeType::Cliff;
    }
    return HexMetrics::GetEdgeType(Elevation, OtherCell->Elevation);
}
//river
void AHexCell::RemoveOutgoingRiver()
{
    if (!bHasOutgoingRiver)
    {
        return;
    }

    bHasOutgoingRiver = false;
    RefreshSelfOnly();

    AHexCell* Neighbor = GetNeighbor(OutgoingRiver);
    if (Neighbor)
    {
        Neighbor->bHasIncomingRiver = false;
        Neighbor->RefreshSelfOnly();
    }

}void AHexCell::RemoveIncomingRiver()
{
    if (!bHasIncomingRiver)
    {
        return;
    }

    bHasIncomingRiver = false;
    RefreshSelfOnly();

    AHexCell* Neighbor = GetNeighbor(IncomingRiver);
    if (Neighbor)
    {
        Neighbor->bHasOutgoingRiver = false;
        Neighbor->RefreshSelfOnly();
    }

}void AHexCell::RemoveRiver()
{
    RemoveOutgoingRiver();
    RemoveIncomingRiver();
}void AHexCell::SetOutgoingRiver(EHexDirection Direction)
{
    if (bHasOutgoingRiver && OutgoingRiver == Direction)
    {
        return;
    }

    AHexCell* Neighbor = GetNeighbor(Direction);
    if (!Neighbor || Elevation < Neighbor->Elevation)
    {
        return; // 无邻居或上坡，退出
    }

    // 清理旧的河流
    RemoveOutgoingRiver();
    if (bHasIncomingRiver && IncomingRiver == Direction)
    {
        RemoveIncomingRiver(); // 如果流入方向和新流出方向重叠，移除流入
    }

    // 设置新流出河流
    bHasOutgoingRiver = true;
    OutgoingRiver = Direction;
    UE_LOG(LogTemp, Log, TEXT("SetOutgoingRiver: Cell (%d, %d) -> Dir %d"), Coordinates.X, Coordinates.Z, static_cast<int32>(Direction));
    RefreshSelfOnly();

    // 设置邻居的流入河流
    Neighbor->RemoveIncomingRiver();
    Neighbor->bHasIncomingRiver = true;
    Neighbor->IncomingRiver = HexMetrics::Opposite(Direction); // 对立方向
    UE_LOG(LogTemp, Log, TEXT("SetIncomingRiver: Neighbor (%d, %d) -> Dir %d"), Neighbor->Coordinates.X, Neighbor->Coordinates.Z, static_cast<int32>(Neighbor->IncomingRiver));
    Neighbor->RefreshSelfOnly();

}void AHexCell::RefreshSelfOnly()
{
    if (Chunk)
    {
        Chunk->Refresh();
    }
}


