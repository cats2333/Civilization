#include "HexGridChunk.h"
#include "HexCell.h"
#include "HexMetrics.h"
#include "Components/DecalComponent.h"
#include "ProceduralMeshComponent.h"
AHexGridChunk::AHexGridChunk()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    HexMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HexMeshComponent"));
    RootComponent = HexMeshComponent;
    HexMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    HexMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Game/Materials/M_HexGrid.M_HexGrid"));
    if (MaterialFinder.Succeeded())
    {
        HexMeshComponent->SetMaterial(0, MaterialFinder.Object);
    }

    // 加载贴花材质
    RoadDecalMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_RoadDecal"));
    if (!RoadDecalMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load M_RoadDecal material! Check path: /Game/Materials/M_RoadDecal"));
        RoadDecalMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultDecalMaterial"));
        if (RoadDecalMaterial)
        {
            UE_LOG(LogTemp, Log, TEXT("Using fallback DefaultDecalMaterial"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load DefaultDecalMaterial!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Successfully loaded M_RoadDecal material in constructor"));
    }

    Cells.SetNum(HexMetrics::ChunkSizeX * HexMetrics::ChunkSizeZ);
}

void AHexGridChunk::BeginPlay()
{
    Super::BeginPlay();
    TriangulateCells();
}

void AHexGridChunk::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    //TriangulateCells();
    SetActorTickEnabled(false);
}

void AHexGridChunk::AddCell(int32 Index, AHexCell* Cell)
{
    Cells[Index] = Cell;
    Cell->Chunk = this;
}

void AHexGridChunk::Refresh()
{
    TriangulateCells();
    UE_LOG(LogTemp, Log, TEXT("AHexGridChunk::Refresh called, triangulating cells"));
}

void AHexGridChunk::TriangulateCells()
{
    ClearMeshData();

    UE_LOG(LogTemp, Log, TEXT("TriangulateCells: Total Cells=%d"), Cells.Num());
    for (AHexCell* Cell : Cells)
    {
        if (!Cell) continue;
        FVector Center = Cell->GetPosition();
        FColor SRGBColor = Cell->Color.ToFColor(true);

        UMaterialInstanceDynamic** DynMaterialPtr = CellMaterials.Find(Cell);
        if (DynMaterialPtr && *DynMaterialPtr)
        {
            HexMeshComponent->SetMaterial(0, *DynMaterialPtr);
        }
        else
        {
            HexMeshComponent->SetMaterial(0, DefaultMaterial);
        }

        bool bHasRoad = Cell->HasRoad();

        if (bHasRoad) {
            UE_LOG(LogTemp, Log, TEXT("Cell (%d, %d) HasRoad: %d"), Cell->Coordinates.X, Cell->Coordinates.Z, bHasRoad);
        }

        for (int32 i = 0; i < 6; i++)
        {
            EHexDirection Direction = static_cast<EHexDirection>(i);
            bool bHasRoadThroughEdge = Cell->HasRoadThroughEdge(Direction);
            if (bHasRoadThroughEdge)
            {
                UE_LOG(LogTemp, Log, TEXT("Cell (%d, %d) HasRoadThroughEdge in direction %d: %d"),
                    Cell->Coordinates.X, Cell->Coordinates.Z, static_cast<int32>(Direction), bHasRoadThroughEdge);
            }
            UE_LOG(LogTemp, Log, TEXT("Checking direction %d for cell (%d, %d): HasRoadThroughEdge=%d"),
                static_cast<int32>(Direction), Cell->Coordinates.X, Cell->Coordinates.Z, bHasRoadThroughEdge);

            HexMetrics::FEdgeVertices E = HexMetrics::FEdgeVertices(
                Center + HexMetrics::GetFirstSolidCorner(Direction),
                Center + HexMetrics::GetSecondSolidCorner(Direction)
            );

            {
                TriangulateEdgeFan(Center, E, SRGBColor);
            }

            if (Direction <= EHexDirection::SE)
            {
                TriangulateConnection(Direction, Cell, E);
            }
        }
    }

    ApplyMesh();
}

void AHexGridChunk::AddTriangle(FVector V1, FVector V2, FVector V3)
{
    int32 VertexIndex = Vertices.Num();
    Vertices.Add(HexMetrics::Perturb(V1));
    Vertices.Add(HexMetrics::Perturb(V2));
    Vertices.Add(HexMetrics::Perturb(V3));

    Normals.Add(FVector(0.0f, 0.0f, 1.0f));
    Normals.Add(FVector(0.0f, 0.0f, 1.0f));
    Normals.Add(FVector(0.0f, 0.0f, 1.0f));

    Triangles.Add(VertexIndex);
    Triangles.Add(VertexIndex + 1);
    Triangles.Add(VertexIndex + 2);
}

void AHexGridChunk::AddTriangleColor(FColor C1, FColor C2, FColor C3)
{
    VertexColors.Add(C1);
    VertexColors.Add(C2);
    VertexColors.Add(C3);
}

void AHexGridChunk::AddTriangleColor(FColor Color)
{
    VertexColors.Add(Color);
    VertexColors.Add(Color);
    VertexColors.Add(Color);
}

void AHexGridChunk::AddQuad(FVector V1, FVector V2, FVector V3, FVector V4)
{
    int32 VertexIndex = Vertices.Num();
    Vertices.Add(HexMetrics::Perturb(V1));
    Vertices.Add(HexMetrics::Perturb(V2));
    Vertices.Add(HexMetrics::Perturb(V3));
    Vertices.Add(HexMetrics::Perturb(V4));

    Normals.Add(FVector(0.0f, 0.0f, 1.0f));
    Normals.Add(FVector(0.0f, 0.0f, 1.0f));
    Normals.Add(FVector(0.0f, 0.0f, 1.0f));
    Normals.Add(FVector(0.0f, 0.0f, 1.0f));

    Triangles.Add(VertexIndex);
    Triangles.Add(VertexIndex + 2);
    Triangles.Add(VertexIndex + 1);
    Triangles.Add(VertexIndex + 1);
    Triangles.Add(VertexIndex + 2);
    Triangles.Add(VertexIndex + 3);
}

void AHexGridChunk::AddQuadColor(FColor C1, FColor C2, FColor C3, FColor C4)
{
    VertexColors.Add(C1);
    VertexColors.Add(C2);
    VertexColors.Add(C3);
    VertexColors.Add(C4);
}

void AHexGridChunk::AddQuadColor(FColor C1, FColor C2)
{
    VertexColors.Add(C1);
    VertexColors.Add(C1);
    VertexColors.Add(C2);
    VertexColors.Add(C2);
}

void AHexGridChunk::AddQuadColor(FColor Color)
{
    VertexColors.Add(Color);
    VertexColors.Add(Color);
    VertexColors.Add(Color);
    VertexColors.Add(Color);
}

void AHexGridChunk::TriangulateEdgeFan(FVector Center, HexMetrics::FEdgeVertices Edge, FColor Color)
{
    AddTriangle(Center, Edge.V1, Edge.V2);
    AddTriangleColor(Color, Color, Color);
    AddTriangle(Center, Edge.V2, Edge.V3);
    AddTriangleColor(Color, Color, Color);
    AddTriangle(Center, Edge.V3, Edge.V4);
    AddTriangleColor(Color, Color, Color);
    AddTriangle(Center, Edge.V4, Edge.V5);
    AddTriangleColor(Color, Color, Color);
}

void AHexGridChunk::TriangulateEdgeStrip(HexMetrics::FEdgeVertices E1, FColor C1, HexMetrics::FEdgeVertices E2, FColor C2)
{
    AddQuad(E1.V1, E1.V2, E2.V1, E2.V2);
    AddQuadColor(C1, C2);
    AddQuad(E1.V2, E1.V3, E2.V2, E2.V3);
    AddQuadColor(C1, C2);
    AddQuad(E1.V3, E1.V4, E2.V3, E2.V4);
    AddQuadColor(C1, C2);
    AddQuad(E1.V4, E1.V5, E2.V4, E2.V5);
    AddQuadColor(C1, C2);
}

void AHexGridChunk::TriangulateConnection(EHexDirection Direction, AHexCell* Cell, HexMetrics::FEdgeVertices E1)
{
    AHexCell* Neighbor = Cell->GetNeighbor(Direction);
    if (!Neighbor) return;

    FVector Bridge = HexMetrics::GetBridge(Direction);
    Bridge.Z = Neighbor->GetPosition().Z - Cell->GetPosition().Z;
    HexMetrics::FEdgeVertices E2 = HexMetrics::FEdgeVertices(E1.V1 + Bridge, E1.V5 + Bridge, 1.0f / 6.0f);

    if (Cell->GetEdgeType(Direction) == HexMetrics::EHexEdgeType::Slope)
    {
        TriangulateEdgeTerraces(E1, Cell, E2, Neighbor);
    }
    else
    {
        TriangulateEdgeStrip(E1, Cell->Color.ToFColor(true), E2, Neighbor->Color.ToFColor(true));
    }

    AHexCell* NextNeighbor = Cell->GetNeighbor(static_cast<EHexDirection>((static_cast<int32>(Direction) + 1) % 6));
    if (Direction <= EHexDirection::E && NextNeighbor)
    {
        FVector V5 = E1.V5 + HexMetrics::GetBridge(static_cast<EHexDirection>((static_cast<int32>(Direction) + 1) % 6));
        V5.Z = NextNeighbor->GetPosition().Z;

        if (Cell->Elevation <= Neighbor->Elevation)
        {
            if (Cell->Elevation <= NextNeighbor->Elevation)
            {
                TriangulateCorner(E1.V5, Cell, E2.V5, Neighbor, V5, NextNeighbor);
            }
            else
            {
                TriangulateCorner(V5, NextNeighbor, E1.V5, Cell, E2.V5, Neighbor);
            }
        }
        else if (Neighbor->Elevation <= NextNeighbor->Elevation)
        {
            TriangulateCorner(E2.V5, Neighbor, V5, NextNeighbor, E1.V5, Cell);
        }
        else
        {
            TriangulateCorner(V5, NextNeighbor, E1.V5, Cell, E2.V5, Neighbor);
        }
    }
}

void AHexGridChunk::TriangulateEdgeTerraces(HexMetrics::FEdgeVertices Begin, AHexCell* BeginCell, HexMetrics::FEdgeVertices End, AHexCell* EndCell)
{
    HexMetrics::FEdgeVertices E2 = HexMetrics::TerraceLerp(Begin, End, 1);
    FLinearColor C2 = HexMetrics::TerraceLerp(BeginCell->Color, EndCell->Color, 1);

    TriangulateEdgeStrip(Begin, BeginCell->Color.ToFColor(true), E2, C2.ToFColor(true));

    for (int32 i = 2; i < HexMetrics::TerraceSteps; i++)
    {
        HexMetrics::FEdgeVertices E1 = E2;
        FLinearColor C1 = C2;
        E2 = HexMetrics::TerraceLerp(Begin, End, i);
        C2 = HexMetrics::TerraceLerp(BeginCell->Color, EndCell->Color, i);
        TriangulateEdgeStrip(E1, C1.ToFColor(true), E2, C2.ToFColor(true));
    }

    TriangulateEdgeStrip(E2, C2.ToFColor(true), End, EndCell->Color.ToFColor(true));
}

void AHexGridChunk::TriangulateCorner(FVector Bottom, AHexCell* BottomCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell)
{
    HexMetrics::EHexEdgeType LeftEdgeType = BottomCell->GetEdgeType(LeftCell);
    HexMetrics::EHexEdgeType RightEdgeType = BottomCell->GetEdgeType(RightCell);

    if (LeftEdgeType == HexMetrics::EHexEdgeType::Slope)
    {
        if (RightEdgeType == HexMetrics::EHexEdgeType::Slope)
        {
            TriangulateCornerTerraces(Bottom, BottomCell, Left, LeftCell, Right, RightCell);
        }
        else if (RightEdgeType == HexMetrics::EHexEdgeType::Flat)
        {
            TriangulateCornerTerraces(Left, LeftCell, Right, RightCell, Bottom, BottomCell);
        }
        else
        {
            TriangulateCornerTerracesCliff(Bottom, BottomCell, Left, LeftCell, Right, RightCell);
        }
    }
    else if (RightEdgeType == HexMetrics::EHexEdgeType::Slope)
    {
        if (LeftEdgeType == HexMetrics::EHexEdgeType::Flat)
        {
            TriangulateCornerTerraces(Right, RightCell, Bottom, BottomCell, Left, LeftCell);
        }
        else
        {
            TriangulateCornerCliffTerraces(Bottom, BottomCell, Left, LeftCell, Right, RightCell);
        }
    }
    else if (LeftCell->GetEdgeType(RightCell) == HexMetrics::EHexEdgeType::Slope)
    {
        if (LeftCell->Elevation < RightCell->Elevation)
        {
            TriangulateCornerCliffTerraces(Right, RightCell, Bottom, BottomCell, Left, LeftCell);
        }
        else
        {
            TriangulateCornerTerracesCliff(Left, LeftCell, Right, RightCell, Bottom, BottomCell);
        }
    }
    else
    {
        AddTriangle(Bottom, Left, Right);
        AddTriangleColor(BottomCell->Color.ToFColor(true), LeftCell->Color.ToFColor(true), RightCell->Color.ToFColor(true));
    }
}

void AHexGridChunk::TriangulateCornerTerraces(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell)
{
    FVector V3 = HexMetrics::TerraceLerp(Begin, Left, 1);
    FVector V4 = HexMetrics::TerraceLerp(Begin, Right, 1);
    FLinearColor C3 = HexMetrics::TerraceLerp(BeginCell->Color, LeftCell->Color, 1);
    FLinearColor C4 = HexMetrics::TerraceLerp(BeginCell->Color, RightCell->Color, 1);

    AddTriangle(Begin, V3, V4);
    AddTriangleColor(BeginCell->Color.ToFColor(true), C3.ToFColor(true), C4.ToFColor(true));

    for (int32 i = 2; i < HexMetrics::TerraceSteps; i++)
    {
        FVector V1 = V3;
        FVector V2 = V4;
        FLinearColor C1 = C3;
        FLinearColor C2 = C4;
        V3 = HexMetrics::TerraceLerp(Begin, Left, i);
        V4 = HexMetrics::TerraceLerp(Begin, Right, i);
        C3 = HexMetrics::TerraceLerp(BeginCell->Color, LeftCell->Color, i);
        C4 = HexMetrics::TerraceLerp(BeginCell->Color, RightCell->Color, i);
        AddQuad(V1, V2, V3, V4);
        AddQuadColor(C1.ToFColor(true), C2.ToFColor(true), C3.ToFColor(true), C4.ToFColor(true));
    }

    AddQuad(V3, V4, Left, Right);
    AddQuadColor(C3.ToFColor(true), C4.ToFColor(true), LeftCell->Color.ToFColor(true), RightCell->Color.ToFColor(true));
}

void AHexGridChunk::TriangulateCornerTerracesCliff(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell)
{
    float B = 1.0f / (RightCell->Elevation - BeginCell->Elevation);
    if (B < 0) B = -B;
    FVector Boundary = FMath::Lerp(HexMetrics::Perturb(Begin), HexMetrics::Perturb(Right), B);
    FLinearColor BoundaryColor = FMath::Lerp(BeginCell->Color, RightCell->Color, B);

    TriangulateBoundaryTriangle(Begin, BeginCell, Left, LeftCell, Boundary, BoundaryColor);

    if (LeftCell->GetEdgeType(RightCell) == HexMetrics::EHexEdgeType::Slope)
    {
        TriangulateBoundaryTriangle(Left, LeftCell, Right, RightCell, Boundary, BoundaryColor);
    }
    else
    {
        AddTriangleUnperturbed(HexMetrics::Perturb(Left), HexMetrics::Perturb(Right), Boundary);
        AddTriangleColor(LeftCell->Color.ToFColor(true), RightCell->Color.ToFColor(true), BoundaryColor.ToFColor(true));
    }
}

void AHexGridChunk::TriangulateCornerCliffTerraces(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell)
{
    float B = 1.0f / (LeftCell->Elevation - BeginCell->Elevation);
    if (B < 0) B = -B;
    FVector Boundary = FMath::Lerp(HexMetrics::Perturb(Begin), HexMetrics::Perturb(Left), B);
    FLinearColor BoundaryColor = FMath::Lerp(BeginCell->Color, LeftCell->Color, B);

    TriangulateBoundaryTriangle(Right, RightCell, Begin, BeginCell, Boundary, BoundaryColor);

    if (LeftCell->GetEdgeType(RightCell) == HexMetrics::EHexEdgeType::Slope)
    {
        TriangulateBoundaryTriangle(Left, LeftCell, Right, RightCell, Boundary, BoundaryColor);
    }
    else
    {
        AddTriangleUnperturbed(HexMetrics::Perturb(Left), HexMetrics::Perturb(Right), Boundary);
        AddTriangleColor(LeftCell->Color.ToFColor(true), RightCell->Color.ToFColor(true), BoundaryColor.ToFColor(true));
    }
}

void AHexGridChunk::TriangulateBoundaryTriangle(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Boundary, FLinearColor BoundaryColor)
{
    FVector V2 = HexMetrics::Perturb(HexMetrics::TerraceLerp(Begin, Left, 1));
    FLinearColor C2 = HexMetrics::TerraceLerp(BeginCell->Color, LeftCell->Color, 1);

    AddTriangleUnperturbed(HexMetrics::Perturb(Begin), V2, Boundary);
    AddTriangleColor(BeginCell->Color.ToFColor(true), C2.ToFColor(true), BoundaryColor.ToFColor(true));

    for (int32 i = 2; i < HexMetrics::TerraceSteps; i++)
    {
        FVector V1 = V2;
        FLinearColor C1 = C2;
        V2 = HexMetrics::Perturb(HexMetrics::TerraceLerp(Begin, Left, i));
        C2 = HexMetrics::TerraceLerp(BeginCell->Color, LeftCell->Color, i);
        AddTriangleUnperturbed(V1, V2, Boundary);
        AddTriangleColor(C1.ToFColor(true), C2.ToFColor(true), BoundaryColor.ToFColor(true));
    }

    AddTriangleUnperturbed(V2, HexMetrics::Perturb(Left), Boundary);
    AddTriangleColor(C2.ToFColor(true), LeftCell->Color.ToFColor(true), BoundaryColor.ToFColor(true));
}

void AHexGridChunk::AddTriangleUnperturbed(FVector V1, FVector V2, FVector V3)
{
    int32 VertexIndex = Vertices.Num();
    Vertices.Add(HexMetrics::Perturb(V1));
    Vertices.Add(HexMetrics::Perturb(V2));
    Vertices.Add(HexMetrics::Perturb(V3));

    Normals.Add(FVector(0.0f, 0.0f, 1.0f));
    Normals.Add(FVector(0.0f, 0.0f, 1.0f));
    Normals.Add(FVector(0.0f, 0.0f, 1.0f));

    Triangles.Add(VertexIndex);
    Triangles.Add(VertexIndex + 1);
    Triangles.Add(VertexIndex + 2);
}

void AHexGridChunk::ClearMeshData()
{
    Vertices.Empty();
    Triangles.Empty();
    Normals.Empty();
    VertexColors.Empty();
    UV0.Empty();
    Tangents.Empty();
}

void AHexGridChunk::ApplyMesh()
{
    UV0.Init(FVector2D(0.0f, 0.0f), Vertices.Num());
    Tangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), Vertices.Num());
    HexMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);
}

void AHexGridChunk::ClearRoadDecals()
{
    UE_LOG(LogTemp, Log, TEXT("Clearing road decals, Total Decals=%d"), RoadDecals.Num());
    for (UDecalComponent* Decal : RoadDecals)
    {
        if (Decal)
        {
            Decal->DestroyComponent();
        }
    }
    RoadDecals.Empty();
}

void AHexGridChunk::CreateRoadDecal(FVector Start, FVector End, float Width, UMaterialInterface* DecalMaterial)
{
    if (!DecalMaterial) return;

    FVector DecalLocation = (Start + End) * 0.5f;
    FVector Direction = (End - Start).GetSafeNormal();
    FRotator DecalRotation = Direction.Rotation();
    float Length = (End - Start).Size();

    FHitResult HitResult;
    FVector TraceStart = DecalLocation + FVector(0, 0, 1000);
    FVector TraceEnd = DecalLocation - FVector(0, 0, 1000);
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
    {
        DecalLocation.Z = HitResult.Location.Z + 0.01f;
    }

    UDecalComponent* Decal = NewObject<UDecalComponent>(this, NAME_None);
    Decal->RegisterComponent();
    Decal->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
    Decal->SetRelativeLocation(DecalLocation);
    Decal->SetRelativeRotation(DecalRotation);
    Decal->DecalSize = FVector(Length * 0.5f, Width, 10.0f);
    Decal->SetDecalMaterial(DecalMaterial);
    Decal->SetFadeScreenSize(0.0f);

    RoadDecals.Add(Decal);
}

void AHexGridChunk::SetCellHighlight(AHexCell* Cell, bool bHighlight)
{
    if (!Cell)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetCellHighlight: Cell is null"));
        return;
    }

    if (bHighlight)
    {
        if (HighlightMaterial)
        {
            UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(HighlightMaterial, this);
            if (DynMaterial)
            {
                DynMaterial->SetVectorParameterValue(FName("EmissiveColor"), FLinearColor(1, 1, 0)); // 默认黄色
                CellMaterials.Add(Cell, DynMaterial);
                UE_LOG(LogTemp, Log, TEXT("Applied M_Highlight to cell (%d, %d)"),
                    Cell->Coordinates.X, Cell->Coordinates.Z);
            }
        }
    }
    else
    {
        CellMaterials.Remove(Cell);
        UE_LOG(LogTemp, Log, TEXT("Removed M_Highlight from cell (%d, %d)"),
            Cell->Coordinates.X, Cell->Coordinates.Z);
    }
}

//void AHexGridChunk::CreateRoadDecal(FVector Start, FVector End, float Width, UMaterialInterface* DecalMaterial)
//{
//    if (!DecalMaterial)
//    {
//        UE_LOG(LogTemp, Warning, TEXT("DecalMaterial is null!"));
//        return;
//    }
//
//    UWorld* World = GetWorld();
//    if (!World)
//    {
//        UE_LOG(LogTemp, Error, TEXT("GetWorld() returned nullptr! Cannot create decal."));
//        return;
//    }
//
//    const int32 MaxDecals = 50;
//    if (RoadDecals.Num() >= MaxDecals)
//    {
//        UDecalComponent* OldDecal = RoadDecals[0];
//        if (OldDecal)
//        {
//            OldDecal->DestroyComponent();
//        }
//        RoadDecals.RemoveAt(0);
//        UE_LOG(LogTemp, Log, TEXT("Removed oldest decal to maintain limit. Current Decals: %d"), RoadDecals.Num());
//    }
//
//    FVector DecalLocation = (Start + End) * 0.5f;
//    FVector Direction = (End - Start).GetSafeNormal();
//    FRotator DecalRotation = Direction.Rotation();
//    DecalRotation.Pitch = 90.0f;
//    float Length = (End - Start).Size();
//
//    // 简化 Z 值计算：假设网格高度为 0，直接抬高
//    DecalLocation.Z = 0.2f; // 默认抬高，避免射线检测
//    /*
//    FHitResult HitResult;
//    FVector TraceStart = DecalLocation + FVector(0, 0, 1000);
//    FVector TraceEnd = DecalLocation - FVector(0, 0, 1000);
//    if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility))
//    {
//        DecalLocation.Z = HitResult.Location.Z + 0.2f;
//        UE_LOG(LogTemp, Log, TEXT("Adjusted Decal Z to: %.3f"), DecalLocation.Z);
//    }
//    else
//    {
//        DecalLocation.Z = 0.2f;
//        UE_LOG(LogTemp, Warning, TEXT("Line trace failed to adjust Decal Z, using default Z=0.2"));
//    }
//    */
//
//    UDecalComponent* Decal = NewObject<UDecalComponent>(this, NAME_None);
//    if (!Decal)
//    {
//        UE_LOG(LogTemp, Error, TEXT("Failed to create UDecalComponent!"));
//        return;
//    }
//
//    Decal->SetRelativeLocation(DecalLocation);
//    Decal->SetRelativeRotation(DecalRotation);
//    Decal->DecalSize = FVector(Length * 0.5f, Width, 10.0f);
//    Decal->SetDecalMaterial(DecalMaterial);
//    Decal->SetFadeScreenSize(0.0f);
//    Decal->SortOrder = RoadDecals.Num() + 1;
//
//    Decal->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
//    Decal->RegisterComponent();
//
//    if (!Decal->IsRegistered())
//    {
//        UE_LOG(LogTemp, Error, TEXT("Decal component failed to register! Decal: %s, Owner: %s"),
//            *GetNameSafe(Decal), *GetNameSafe(Decal->GetOwner()));
//        return;
//    }
//
//    RoadDecals.Add(Decal);
//
//    UE_LOG(LogTemp, Log, TEXT("Created road decal: Start=(%.3f, %.3f, %.3f), End=(%.3f, %.3f, %.3f), Length=%.3f, DecalSize=(%.3f, %.3f, %.3f)"),
//        Start.X, Start.Y, Start.Z, End.X, End.Y, End.Z, Length,
//        Decal->DecalSize.X, Decal->DecalSize.Y, Decal->DecalSize.Z);
//    UE_LOG(LogTemp, Log, TEXT("Total RoadDecals after creation: %d"), RoadDecals.Num());
//}