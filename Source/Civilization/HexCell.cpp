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

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    Color = FLinearColor::White;
    bHasOutgoingRoad = false;
    bHasIncomingRoad = false;
    bIsHighlighted = false;
    Elevation = 0;

    HighlightMeshComponent = nullptr;
    HighlightMaterial = nullptr; // 初始化为空，靠蓝图设置
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

void AHexCell::SetHighlighted(bool bHighlight)
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
                UE_LOG(LogTemp, Warning, TEXT("No HighlightMaterial set in BP_Cell for cell (%d, %d), using default material"),
                    Coordinates.X, Coordinates.Z);
            }
            HighlightMeshComponent->SetMaterial(0, MaterialToUse);

            // 启用 Custom Depth 调试
            HighlightMeshComponent->SetRenderCustomDepth(true);
            HighlightMeshComponent->CustomDepthStencilValue = 1;
        }

        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FColor> VertexColors;
        TArray<FVector2D> UV0;
        TArray<FProcMeshTangent> Tangents;

        FVector Center = GetPosition();
        float HighlightOffsetZ = 1.0f + Elevation * 0.5f; // 动态调整 Z 偏移
        float OutlineWidth = 0.5f; // 描边宽度

        // 打印中心点和 HexMetrics 数据
        UE_LOG(LogTemp, Log, TEXT("Cell (%d, %d) Center Position = %s, Elevation = %d"),
            Coordinates.X, Coordinates.Z, *Center.ToString(), Elevation);
        UE_LOG(LogTemp, Log, TEXT("HexMetrics::OuterRadius = %f, OutlineWidth = %f"), HexMetrics::OuterRadius, OutlineWidth);

        // 构造描边 Mesh（内外边框）
        for (int32 i = 0; i < 6; i++)
        {
            FVector OuterCorner = Center + HexMetrics::Corners[i];
            FVector InnerCorner = Center + HexMetrics::Corners[i] * (1.0f - OutlineWidth / HexMetrics::OuterRadius);
            OuterCorner.Z += HighlightOffsetZ;
            InnerCorner.Z += HighlightOffsetZ;

            int32 NextI = (i + 1) % 6;
            FVector NextOuterCorner = Center + HexMetrics::Corners[NextI];
            FVector NextInnerCorner = Center + HexMetrics::Corners[NextI] * (1.0f - OutlineWidth / HexMetrics::OuterRadius);
            NextOuterCorner.Z += HighlightOffsetZ;
            NextInnerCorner.Z += HighlightOffsetZ;

            int32 VertexIndex = Vertices.Num();
            Vertices.Add(OuterCorner);
            Vertices.Add(InnerCorner);
            Vertices.Add(NextOuterCorner);
            Vertices.Add(NextInnerCorner);

            // 打印顶点数据
            UE_LOG(LogTemp, Log, TEXT("Vertex %d (OuterCorner): %s"), VertexIndex, *OuterCorner.ToString());
            UE_LOG(LogTemp, Log, TEXT("Vertex %d (InnerCorner): %s"), VertexIndex + 1, *InnerCorner.ToString());
            UE_LOG(LogTemp, Log, TEXT("Vertex %d (NextOuterCorner): %s"), VertexIndex + 2, *NextOuterCorner.ToString());
            UE_LOG(LogTemp, Log, TEXT("Vertex %d (NextInnerCorner): %s"), VertexIndex + 3, *NextInnerCorner.ToString());

            Normals.Add(FVector(0, 0, 1));
            Normals.Add(FVector(0, 0, 1));
            Normals.Add(FVector(0, 0, 1));
            Normals.Add(FVector(0, 0, 1));

            FColor HighlightColor = FColor::Yellow;
            VertexColors.Add(HighlightColor);
            VertexColors.Add(HighlightColor);
            VertexColors.Add(HighlightColor);
            VertexColors.Add(HighlightColor);

            Triangles.Add(VertexIndex);
            Triangles.Add(VertexIndex + 1);
            Triangles.Add(VertexIndex + 2);
            Triangles.Add(VertexIndex + 1);
            Triangles.Add(VertexIndex + 3);
            Triangles.Add(VertexIndex + 2);

            // 打印三角形索引
            UE_LOG(LogTemp, Log, TEXT("Triangle 1: %d, %d, %d"), VertexIndex, VertexIndex + 1, VertexIndex + 2);
            UE_LOG(LogTemp, Log, TEXT("Triangle 2: %d, %d, %d"), VertexIndex + 1, VertexIndex + 3, VertexIndex + 2);
        }

        UV0.Init(FVector2D(0, 0), Vertices.Num());
        Tangents.Init(FProcMeshTangent(1, 0, 0), Vertices.Num());

        // 打印 Mesh 数据总数
        UE_LOG(LogTemp, Log, TEXT("Vertices Count = %d, Triangles Count = %d, Normals Count = %d"),
            Vertices.Num(), Triangles.Num(), Normals.Num());

        HighlightMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
        HighlightMeshComponent->SetVisibility(true);

        UE_LOG(LogTemp, Log, TEXT("Highlighted cell (%d, %d) with material %s"),
            Coordinates.X, Coordinates.Z, HighlightMaterial ? *HighlightMaterial->GetName() : TEXT("None"));
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

//void AHexCell::SetHighlighted(bool bHighlight)
//{
//    if (bIsHighlighted == bHighlight)
//    {
//        return;
//    }
//
//    bIsHighlighted = bHighlight;
//
//    if (bIsHighlighted)
//    {
//        UE_LOG(LogTemp, Log, TEXT("HexMetrics::OuterRadius = %f"), HexMetrics::OuterRadius);
//        if (!HighlightMeshComponent)
//        {
//            HighlightMeshComponent = NewObject<UProceduralMeshComponent>(this, TEXT("HighlightMesh"));
//            HighlightMeshComponent->RegisterComponent();
//            HighlightMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
//
//            // 使用蓝图配置的材质
//            UMaterialInterface* MaterialToUse = HighlightMaterial;
//            if (!MaterialToUse)
//            {
//                // 默认材质回退
//                MaterialToUse = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultDecalMaterial"));
//                UE_LOG(LogTemp, Warning, TEXT("No HighlightMaterial set in BP_Cell for cell (%d, %d), using default material"),
//                    Coordinates.X, Coordinates.Z);
//            }
//            HighlightMeshComponent->SetMaterial(0, MaterialToUse);
//
//            // 确保双面渲染
//            HighlightMeshComponent->bUseComplexAsSimpleCollision = true;
//            HighlightMeshComponent->SetRenderCustomDepth(true); // 可视化调试
//            HighlightMeshComponent->CustomDepthStencilValue = 1;
//        }
//
//        // 构造高亮 Mesh（空心描边）
//        TArray<FVector> Vertices;
//        TArray<int32> Triangles;
//        TArray<FVector> Normals;
//        TArray<FColor> VertexColors;
//        TArray<FVector2D> UV0;
//        TArray<FProcMeshTangent> Tangents;
//
//        FVector Center = GetPosition();
//        float HighlightOffsetZ = 1.0f; // 抬高避免重叠0.1
//        float OutlineWidth = 0.5f; // 描边宽度//0.05
//
//        UE_LOG(LogTemp, Log, TEXT("Cell (%d, %d) Elevation = %d, Z Position = %f"),
//            Coordinates.X, Coordinates.Z, Elevation, Center.Z);
//        UE_LOG(LogTemp, Log, TEXT("HexMetrics::OuterRadius = %f, OutlineWidth = %f"), HexMetrics::OuterRadius, OutlineWidth);
//        for (int32 i = 0; i < 6; i++)
//        {
//            FVector OuterCorner = Center + HexMetrics::Corners[i];
//            FVector InnerCorner = Center + HexMetrics::Corners[i] * (1.0f - OutlineWidth / HexMetrics::OuterRadius);
//            OuterCorner.Z += HighlightOffsetZ;
//            InnerCorner.Z += HighlightOffsetZ;
//
//            int32 NextI = (i + 1) % 6;
//            FVector NextOuterCorner = Center + HexMetrics::Corners[NextI];
//            FVector NextInnerCorner = Center + HexMetrics::Corners[NextI] * (1.0f - OutlineWidth / HexMetrics::OuterRadius);
//            NextOuterCorner.Z += HighlightOffsetZ;
//            NextInnerCorner.Z += HighlightOffsetZ;
//
//            int32 VertexIndex = Vertices.Num();
//            Vertices.Add(OuterCorner);
//            Vertices.Add(InnerCorner);
//            Vertices.Add(NextOuterCorner);
//            Vertices.Add(NextInnerCorner);
//
//            Normals.Add(FVector(0, 0, 1));
//            Normals.Add(FVector(0, 0, 1));
//            Normals.Add(FVector(0, 0, 1));
//            Normals.Add(FVector(0, 0, 1));
//
//            FColor HighlightColor = FColor::Yellow; // 可调整
//            VertexColors.Add(HighlightColor);
//            VertexColors.Add(HighlightColor);
//            VertexColors.Add(HighlightColor);
//            VertexColors.Add(HighlightColor);
//
//            Triangles.Add(VertexIndex);
//            Triangles.Add(VertexIndex + 1);
//            Triangles.Add(VertexIndex + 2);
//            Triangles.Add(VertexIndex + 1);
//            Triangles.Add(VertexIndex + 3);
//            Triangles.Add(VertexIndex + 2);
//        }
//
//        UV0.Init(FVector2D(0, 0), Vertices.Num());
//        Tangents.Init(FProcMeshTangent(1, 0, 0), Vertices.Num());
//
//        HighlightMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
//        HighlightMeshComponent->SetVisibility(true);
//
//        UE_LOG(LogTemp, Log, TEXT("Highlighted cell (%d, %d)"), Coordinates.X, Coordinates.Z);
//    }
//    else
//    {
//        if (HighlightMeshComponent)
//        {
//            HighlightMeshComponent->ClearAllMeshSections();
//            HighlightMeshComponent->SetVisibility(false);
//            HighlightMeshComponent->DestroyComponent();
//            HighlightMeshComponent = nullptr;
//        }
//
//        UE_LOG(LogTemp, Log, TEXT("Removed highlight from cell (%d, %d)"), Coordinates.X, Coordinates.Z);
//    }
//}
