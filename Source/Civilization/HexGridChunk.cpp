#include "HexGridChunk.h"
#include "HexCell.h"
#include "HexMetrics.h"
AHexGridChunk::AHexGridChunk()
{
    PrimaryActorTick.bCanEverTick = true; // 启用 Tick
    PrimaryActorTick.bStartWithTickEnabled = false; // 默认不启用 Tick

    HexMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HexMeshComponent"));
    RootComponent = HexMeshComponent;
    HexMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    HexMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

    RiverMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RiverMeshComponent"));
    RiverMeshComponent->SetupAttachment(RootComponent);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Game/Materials/M_HexGrid.M_HexGrid"));
    if (MaterialFinder.Succeeded())
    {
        HexMeshComponent->SetMaterial(0, MaterialFinder.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> RiverMaterialFinder(TEXT("/Game/Materials/M_River.M_River"));
    if (RiverMaterialFinder.Succeeded())
    {
        RiverMeshComponent->SetMaterial(0, RiverMaterialFinder.Object);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load M_River material! Check path: /Game/Materials/M_River"));
    }
    Cells.SetNum(HexMetrics::ChunkSizeX* HexMetrics::ChunkSizeZ);

}void AHexGridChunk::BeginPlay()
{
    Super::BeginPlay();
    TriangulateCells();
}

void AHexGridChunk::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    TriangulateCells();
    SetActorTickEnabled(false);
}


void AHexGridChunk::AddCell(int32 Index, AHexCell* Cell)
{
    Cells[Index] = Cell;
    Cell->Chunk = this;
}void AHexGridChunk::Refresh()
{
    SetActorTickEnabled(true);

}

void AHexGridChunk::TriangulateCells()
{
    ClearMeshData();

    for (AHexCell* Cell : Cells)
    {
        if (!Cell) continue;
        FVector Center = Cell->GetPosition();
        FColor SRGBColor = Cell->Color.ToFColor(true);

        for (int32 i = 0; i < 6; i++)
        {
            EHexDirection Direction = static_cast<EHexDirection>(i);
            HexMetrics::FEdgeVertices E = HexMetrics::FEdgeVertices(
                Center + HexMetrics::GetFirstSolidCorner(Direction),
                Center + HexMetrics::GetSecondSolidCorner(Direction)
            );
            UE_LOG(LogTemp, Log, TEXT("Cell (%d, %d) has river"), Cell->Coordinates.X, Cell->Coordinates.Z);
            if (Cell->HasRiverThroughEdge(Direction))
            {
                UE_LOG(LogTemp, Log, TEXT("Triangulating with river in direction %d"), static_cast<int32>(Direction));
                HexMetrics::FEdgeVertices RiverE = E;
                RiverE.V3.Z = Cell->GetStreamBedZ();
                if (Cell->HasRiver())
                {
                    if (Cell->HasRiverBeginOrEnd())
                    {
                        TriangulateWithRiverBeginOrEnd(Direction, Cell, Center, RiverE);
                    }
                    else
                    {
                        TriangulateWithRiver(Direction, Cell, Center, RiverE);
                    }
                }
                /*else
                {
                    TriangulateRiverEdgeFan(Center, RiverE, SRGBColor);
                }*/
            }
            else
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
}void AHexGridChunk::AddQuad(FVector V1, FVector V2, FVector V3, FVector V4)
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

}void AHexGridChunk::AddQuadColor(FColor C1, FColor C2, FColor C3, FColor C4)
{
    VertexColors.Add(C1);
    VertexColors.Add(C2);
    VertexColors.Add(C3);
    VertexColors.Add(C4);
}void AHexGridChunk::AddQuadColor(FColor C1, FColor C2)
{
    VertexColors.Add(C1);
    VertexColors.Add(C1);
    VertexColors.Add(C2);
    VertexColors.Add(C2);
}void AHexGridChunk::TriangulateEdgeFan(FVector Center, HexMetrics::FEdgeVertices Edge, FColor Color)
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

void AHexGridChunk::TriangulateRiverEdgeFan(FVector Center, HexMetrics::FEdgeVertices Edge, FColor Color)
{
    AddRiverTriangle(Center, Edge.V1, Edge.V2);
    AddRiverTriangleColor(Color, Color, Color);
    AddRiverTriangle(Center, Edge.V2, Edge.V3);
    AddRiverTriangleColor(Color, Color, Color);
    AddRiverTriangle(Center, Edge.V3, Edge.V4);
    AddRiverTriangleColor(Color, Color, Color);
    AddRiverTriangle(Center, Edge.V4, Edge.V5);
    AddRiverTriangleColor(Color, Color, Color);
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
    //HexMetrics::FEdgeVertices E2 = HexMetrics::FEdgeVertices(E1.V1 + Bridge, E1.V5 + Bridge);
    HexMetrics::FEdgeVertices E2 = HexMetrics::FEdgeVertices(E1.V1 + Bridge, E1.V5 + Bridge, 1.0f / 6.0f); // 使用 outerStep

    if (Cell->HasRiverThroughEdge(Direction))
    {
        E2.V3.Z = Neighbor->GetStreamBedZ();
        TriangulateEdgeStrip(E1, Cell->Color.ToFColor(true), E2, Neighbor->Color.ToFColor(true));

        /*AddRiverQuad(E1.V1, E1.V2, E2.V1, E2.V2);
        AddRiverQuadColor(Cell->Color.ToFColor(true), Neighbor->Color.ToFColor(true));
        AddRiverQuad(E1.V2, E1.V3, E2.V2, E2.V3);
        AddRiverQuadColor(Cell->Color.ToFColor(true), Neighbor->Color.ToFColor(true));
        AddRiverQuad(E1.V3, E1.V4, E2.V3, E2.V4);
        AddRiverQuadColor(Cell->Color.ToFColor(true), Neighbor->Color.ToFColor(true));
        AddRiverQuad(E1.V4, E1.V5, E2.V4, E2.V5);
        AddRiverQuadColor(Cell->Color.ToFColor(true), Neighbor->Color.ToFColor(true));*/
    }

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
    Vertices.Add(HexMetrics::Perturb(V1)); // 统一应用扰动
    Vertices.Add(HexMetrics::Perturb(V2));
    Vertices.Add(HexMetrics::Perturb(V3));

    Normals.Add(FVector(0.0f, 0.0f, 1.0f));
    Normals.Add(FVector(0.0f, 0.0f, 1.0f));
    Normals.Add(FVector(0.0f, 0.0f, 1.0f));

    Triangles.Add(VertexIndex);
    Triangles.Add(VertexIndex + 1);
    Triangles.Add(VertexIndex + 2);
}
//void AHexGridChunk::ShowChunkUI(bool bVisible)
//{
//    /* if (GridCanvas)
//     {
//         GridCanvas->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
//     }*/
//}

void AHexGridChunk::ClearMeshData()
{
    Vertices.Empty();
    Triangles.Empty();
    Normals.Empty();
    VertexColors.Empty();
    UV0.Empty();
    Tangents.Empty();

    RiverVertices.Empty();
    RiverTriangles.Empty();
    RiverNormals.Empty();
    RiverVertexColors.Empty();
    RiverUV0.Empty();
    RiverTangents.Empty();
}

void AHexGridChunk::ApplyMesh()
{
    UV0.Init(FVector2D(0.0f, 0.0f), Vertices.Num());
    Tangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), Vertices.Num());
    HexMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, true);

    RiverUV0.Init(FVector2D(0.0f, 0.0f), RiverVertices.Num());
    RiverTangents.Init(FProcMeshTangent(1.0f, 0.0f, 0.0f), RiverVertices.Num());
    RiverMeshComponent->CreateMeshSection(0, RiverVertices, RiverTriangles, RiverNormals, RiverUV0, RiverVertexColors, RiverTangents, true);
}

void AHexGridChunk::AddRiverTriangle(FVector V1, FVector V2, FVector V3)
{
    int32 VertexIndex = RiverVertices.Num();
    RiverVertices.Add(HexMetrics::Perturb(V1));
    RiverVertices.Add(HexMetrics::Perturb(V2));
    RiverVertices.Add(HexMetrics::Perturb(V3));

    RiverNormals.Add(FVector(0.0f, 0.0f, 1.0f));
    RiverNormals.Add(FVector(0.0f, 0.0f, 1.0f));
    RiverNormals.Add(FVector(0.0f, 0.0f, 1.0f));

    RiverTriangles.Add(VertexIndex);
    RiverTriangles.Add(VertexIndex + 1);
    RiverTriangles.Add(VertexIndex + 2);
}

void AHexGridChunk::AddRiverTriangleColor(FColor C1, FColor C2, FColor C3)
{
    RiverVertexColors.Add(C1);
    RiverVertexColors.Add(C2);
    RiverVertexColors.Add(C3);
}

void AHexGridChunk::AddRiverQuad(FVector V1, FVector V2, FVector V3, FVector V4)
{
    int32 VertexIndex = RiverVertices.Num();
    RiverVertices.Add(HexMetrics::Perturb(V1));
    RiverVertices.Add(HexMetrics::Perturb(V2));
    RiverVertices.Add(HexMetrics::Perturb(V3));
    RiverVertices.Add(HexMetrics::Perturb(V4));

    RiverNormals.Add(FVector(0.0f, 0.0f, 1.0f));
    RiverNormals.Add(FVector(0.0f, 0.0f, 1.0f));
    RiverNormals.Add(FVector(0.0f, 0.0f, 1.0f));
    RiverNormals.Add(FVector(0.0f, 0.0f, 1.0f));

    RiverTriangles.Add(VertexIndex);
    RiverTriangles.Add(VertexIndex + 1);
    RiverTriangles.Add(VertexIndex + 2);
    RiverTriangles.Add(VertexIndex + 1);
    RiverTriangles.Add(VertexIndex + 3);
    RiverTriangles.Add(VertexIndex + 2);
}

void AHexGridChunk::AddRiverQuadColor(FColor C1, FColor C2, FColor C3, FColor C4)
{
    RiverVertexColors.Add(C1);
    RiverVertexColors.Add(C2);
    RiverVertexColors.Add(C3);
    RiverVertexColors.Add(C4);
}

void AHexGridChunk::AddRiverQuadColor(FColor C1, FColor C2)
{
    RiverVertexColors.Add(C1);
    RiverVertexColors.Add(C1);
    RiverVertexColors.Add(C2);
    RiverVertexColors.Add(C2);
}

void AHexGridChunk::TriangulateWithRiver(EHexDirection Direction, AHexCell* Cell, FVector Center, HexMetrics::FEdgeVertices E) {
    FVector CenterL, CenterR;
    if (Cell->HasRiverThroughEdge(HexMetrics::Opposite(Direction))) {
        UE_LOG(LogTemp, Log, TEXT("Straight river in direction %d"), static_cast<int32>(Direction));
        CenterL = Center + HexMetrics::GetFirstSolidCorner(HexMetrics::Previous(Direction)) * 0.25f;
        CenterR = Center + HexMetrics::GetSecondSolidCorner(HexMetrics::Next(Direction)) * 0.25f;
    }
    else if (Cell->HasRiverThroughEdge(HexMetrics::Next(Direction))) {
        CenterL = Center;
        CenterR = FVector::VectorPlaneProject(FMath::Lerp(Center, E.V5, 2.0f / 3.0f), FVector::UpVector);
    }
    else if (Cell->HasRiverThroughEdge(HexMetrics::Previous(Direction))) {
        CenterL = FVector::VectorPlaneProject(FMath::Lerp(Center, E.V1, 2.0f / 3.0f), FVector::UpVector);
        CenterR = Center;
    }
    else {
        CenterL = CenterR = Center;
    }
    UE_LOG(LogTemp, Log, TEXT("CenterL: %s, CenterR: %s"), *CenterL.ToString(), *CenterR.ToString());
    //HexMetrics::FEdgeVertices M(CenterL, CenterR);
    Center = FMath::Lerp(CenterL, CenterR, 0.5f);

    HexMetrics::FEdgeVertices M(
        FMath::Lerp(CenterL, E.V1, 0.5f),
        FMath::Lerp(CenterR, E.V5, 0.5f),
        1.0f / 6.0f
    );
    M.V3.Z = Center.Z = E.V3.Z;

    TriangulateEdgeStrip(M, Cell->Color.ToFColor(true), E, Cell->Color.ToFColor(true));
    AddTriangle(CenterL, M.V1, M.V2);
    AddTriangleColor(Cell->Color.ToFColor(true));
    AddQuad(CenterL, Center, M.V2, M.V3);
    AddQuadColor(Cell->Color.ToFColor(true));
    AddQuad(Center, CenterR, M.V3, M.V4);
    AddQuadColor(Cell->Color.ToFColor(true));
    AddTriangle(CenterR, M.V4, M.V5);
    AddTriangleColor(Cell->Color.ToFColor(true));
}

void AHexGridChunk::AddTriangleColor(FColor Color)
{
    VertexColors.Add(Color);
    VertexColors.Add(Color);
    VertexColors.Add(Color);
}

void AHexGridChunk::AddQuadColor(FColor Color)
{
    VertexColors.Add(Color);
    VertexColors.Add(Color);
    VertexColors.Add(Color);
    VertexColors.Add(Color);
}

void AHexGridChunk::TriangulateWithRiverBeginOrEnd(EHexDirection Direction, AHexCell* Cell, FVector Center, HexMetrics::FEdgeVertices E)
{
    HexMetrics::FEdgeVertices M(
        FMath::Lerp(Center, E.V1, 0.5f),
        FMath::Lerp(Center, E.V5, 0.5f),
        1.0f / 6.0f
    );
    M.V3.Z = E.V3.Z; // 仅调整中间顶点高度

    TriangulateEdgeStrip(M, Cell->Color.ToFColor(true), E, Cell->Color.ToFColor(true));
    TriangulateEdgeFan(Center, M, Cell->Color.ToFColor(true));
}

//void AHexGridChunk::TriangulateAdjacentToRiver(EHexDirection Direction, AHexCell* Cell, FVector Center, HexMetrics::FEdgeVertices E)
//{
//    FVector AdjustedCenter = Center;
//
//    // 调整中心点以匹配河流形状
//    if (Cell->HasRiverThroughEdge(HexMetrics::Next(Direction)))
//    {
//        if (Cell->HasRiverThroughEdge(HexMetrics::Previous(Direction)))
//        {
//            AdjustedCenter += HexMetrics::GetSolidEdgeMiddle(Direction) * (HexMetrics::innerToOuter * 0.5f);
//        }
//        else if (Cell->HasRiverThroughEdge(HexMetrics::Previous2(Direction)))
//        {
//            AdjustedCenter += HexMetrics::GetFirstSolidCorner(Direction) * 0.25f;
//        }
//    }
//    else if (Cell->HasRiverThroughEdge(HexMetrics::Previous(Direction)) &&
//        Cell->HasRiverThroughEdge(HexMetrics::Next2(Direction)))
//    {
//        AdjustedCenter += HexMetrics::GetSecondSolidCorner(Direction) * 0.25f;
//    }
//
//    HexMetrics::FEdgeVertices M(
//        FMath::Lerp(AdjustedCenter, E.V1, 0.5f),
//        FMath::Lerp(AdjustedCenter, E.V5, 0.5f),
//        1.0f / 6.0f
//    );
//
//    TriangulateEdgeStrip(M, Cell->Color.ToFColor(true), E, Cell->Color.ToFColor(true));
//    TriangulateEdgeFan(AdjustedCenter, M, Cell->Color.ToFColor(true));
//}