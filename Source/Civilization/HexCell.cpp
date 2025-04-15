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
//        if (!HighlightMeshComponent)
//        {
//            HighlightMeshComponent = NewObject<UProceduralMeshComponent>(this, TEXT("HighlightMesh"));
//            HighlightMeshComponent->RegisterComponent();
//            HighlightMeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
//
//            UMaterialInterface* MaterialToUse = HighlightMaterial;
//            if (!MaterialToUse)
//            {
//                MaterialToUse = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultDecalMaterial"));
//                UE_LOG(LogTemp, Warning, TEXT("No HighlightMaterial set in BP_Cell for cell (%d, %d), using default material"),
//                    Coordinates.X, Coordinates.Z);
//            }
//            HighlightMeshComponent->SetMaterial(0, MaterialToUse);
//
//            // 启用 Custom Depth 调试
//            HighlightMeshComponent->SetRenderCustomDepth(true);
//            HighlightMeshComponent->CustomDepthStencilValue = 1;
//        }
//
//        TArray<FVector> Vertices;
//        TArray<int32> Triangles;
//        TArray<FVector> Normals;
//        TArray<FColor> VertexColors;
//        TArray<FVector2D> UV0;
//        TArray<FProcMeshTangent> Tangents;
//
//        FVector Center = GetPosition();
//        float HighlightOffsetZ = 1.0f + Elevation * 0.5f; // 动态调整 Z 偏移
//        float OutlineWidth = 0.5f; // 描边宽度
//
//        // 打印中心点和 HexMetrics 数据
//        UE_LOG(LogTemp, Log, TEXT("Cell (%d, %d) Center Position = %s, Elevation = %d"),
//            Coordinates.X, Coordinates.Z, *Center.ToString(), Elevation);
//        UE_LOG(LogTemp, Log, TEXT("HexMetrics::OuterRadius = %f, OutlineWidth = %f"), HexMetrics::OuterRadius, OutlineWidth);
//
//        // 构造描边 Mesh（内外边框）
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
//            // 打印顶点数据
//            UE_LOG(LogTemp, Log, TEXT("Vertex %d (OuterCorner): %s"), VertexIndex, *OuterCorner.ToString());
//            UE_LOG(LogTemp, Log, TEXT("Vertex %d (InnerCorner): %s"), VertexIndex + 1, *InnerCorner.ToString());
//            UE_LOG(LogTemp, Log, TEXT("Vertex %d (NextOuterCorner): %s"), VertexIndex + 2, *NextOuterCorner.ToString());
//            UE_LOG(LogTemp, Log, TEXT("Vertex %d (NextInnerCorner): %s"), VertexIndex + 3, *NextInnerCorner.ToString());
//
//            Normals.Add(FVector(0, 0, 1));
//            Normals.Add(FVector(0, 0, 1));
//            Normals.Add(FVector(0, 0, 1));
//            Normals.Add(FVector(0, 0, 1));
//
//            FColor HighlightColor = FColor::Yellow;
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
//
//            // 打印三角形索引
//            UE_LOG(LogTemp, Log, TEXT("Triangle 1: %d, %d, %d"), VertexIndex, VertexIndex + 1, VertexIndex + 2);
//            UE_LOG(LogTemp, Log, TEXT("Triangle 2: %d, %d, %d"), VertexIndex + 1, VertexIndex + 3, VertexIndex + 2);
//        }
//
//        UV0.Init(FVector2D(0, 0), Vertices.Num());
//        Tangents.Init(FProcMeshTangent(1, 0, 0), Vertices.Num());
//
//        // 打印 Mesh 数据总数
//        UE_LOG(LogTemp, Log, TEXT("Vertices Count = %d, Triangles Count = %d, Normals Count = %d"),
//            Vertices.Num(), Triangles.Num(), Normals.Num());
//
//        HighlightMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);
//        HighlightMeshComponent->SetVisibility(true);
//
//        UE_LOG(LogTemp, Log, TEXT("Highlighted cell (%d, %d) with material %s"),
//            Coordinates.X, Coordinates.Z, HighlightMaterial ? *HighlightMaterial->GetName() : TEXT("None"));
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
                    Coordinates.X, Coordinates.Y);
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

        // 通过 Coordinates 计算格子的局部坐标（相对于 AHexGrid）
        int32 X = Coordinates.X + (Coordinates.Z - (Coordinates.Z & 1)) / 2; // 偏移坐标 X
        int32 Z = Coordinates.Z; // 偏移坐标 Z
        const float SpacingFactor = 1.0f;
        float PosX = (X + Z * 0.5f - Z / 2) * (HexMetrics::InnerRadius * 2.0f) * SpacingFactor;
        float PosY = Z * (HexMetrics::OuterRadius * 1.5f) * SpacingFactor;
        FVector LocalPosition(PosX, PosY, 0.0f);

        // 转换为世界坐标（反向 X 和 Y 轴）
        FVector WorldPosition = LocalPosition;
        // 调整为 (X, Y) -> (-Y, X)
        float TempX = LocalPosition.X;
        WorldPosition.X = -LocalPosition.Y;
        WorldPosition.Y = LocalPosition.X;

        // 由于 HighlightMeshComponent 使用 KeepRelativeTransform，转换回 AHexCell 的局部坐标
        FVector Center = GetActorTransform().InverseTransformPosition(WorldPosition);

        float HighlightOffsetZ = 0.0f; // 调低高度
        float OutlineWidth = 0.2f; // 描边宽度
        float OuterRadius = HexMetrics::OuterRadius; // 六边形外径
        float InnerRadius = OuterRadius * FMath::Sqrt(3.0f) / 2.0f; // 六边形内径

        UE_LOG(LogTemp, Log, TEXT("Coordinates = (%d, %d, %d), LocalPosition = %s, WorldPosition = %s, Center (Relative) = %s"),
            Coordinates.X, Coordinates.Y, Coordinates.Z,
            *LocalPosition.ToString(),
            *WorldPosition.ToString(),
            *Center.ToString());

        // 手动定义六边形顶点（尖顶布局，匹配格子排列方向）
        TArray<FVector> Corners;
        Corners.SetNum(6);
        // 尖顶布局的六边形顶点（顺时针，从东北开始）
        Corners[0] = FVector(0.0f, OuterRadius, 0.0f);           // NE (尖顶)
        Corners[1] = FVector(InnerRadius, OuterRadius / 2, 0.0f); // E
        Corners[2] = FVector(InnerRadius, -OuterRadius / 2, 0.0f); // SE
        Corners[3] = FVector(0.0f, -OuterRadius, 0.0f);          // SW
        Corners[4] = FVector(-InnerRadius, -OuterRadius / 2, 0.0f); // W
        Corners[5] = FVector(-InnerRadius, OuterRadius / 2, 0.0f);  // NW

        // 构造空心描边
        for (int32 i = 0; i < 6; i++)
        {
            // 外顶点和内顶点
            FVector OuterCorner = Center + Corners[i];
            FVector InnerCorner = Center + Corners[i] * (1.0f - OutlineWidth / OuterRadius);

            int32 NextI = (i + 1) % 6;
            FVector NextOuterCorner = Center + Corners[NextI];
            FVector NextInnerCorner = Center + Corners[NextI] * (1.0f - OutlineWidth / OuterRadius);

            OuterCorner.Z += HighlightOffsetZ;
            InnerCorner.Z += HighlightOffsetZ;
            NextOuterCorner.Z += HighlightOffsetZ;
            NextInnerCorner.Z += HighlightOffsetZ;

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