#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "HexDirection.h"
#include "HexMetrics.h"
#include "HexGridChunk.generated.h"

class AHexCell;

UCLASS()
class CIVILIZATION_API AHexGridChunk : public AActor
{
    GENERATED_BODY()

public:
    AHexGridChunk();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    void AddCell(int32 Index, AHexCell* Cell);
    void Refresh();
    void TriangulateCells();

    //UFUNCTION(BlueprintImplementableEvent, Category = "HexGridChunk")
    //void ShowChunkUI(bool bVisible);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UProceduralMeshComponent* HexMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HexGrid")
    UProceduralMeshComponent* RiverMeshComponent;

    UPROPERTY()
    TArray<AHexCell*> Cells;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    TArray<FVector> RiverVertices;
    TArray<int32> RiverTriangles;
    TArray<FVector> RiverNormals;
    TArray<FColor> RiverVertexColors;
    TArray<FVector2D> RiverUV0;
    TArray<FProcMeshTangent> RiverTangents;
private:
    void AddTriangle(FVector V1, FVector V2, FVector V3);
    void AddTriangleColor(FColor C1, FColor C2, FColor C3);
    void AddQuad(FVector V1, FVector V2, FVector V3, FVector V4);
    void AddQuadColor(FColor C1, FColor C2, FColor C3, FColor C4);
    void AddQuadColor(FColor C1, FColor C2);
    void AddTriangleUnperturbed(FVector V1, FVector V2, FVector V3);

    void ClearMeshData();
    void ApplyMesh();

    void AddRiverTriangle(FVector V1, FVector V2, FVector V3);
    void AddRiverTriangleColor(FColor C1, FColor C2, FColor C3); // 新增
    void AddRiverQuad(FVector V1, FVector V2, FVector V3, FVector V4);
    void AddRiverQuadColor(FColor C1, FColor C2, FColor C3, FColor C4); // 新增
    void AddRiverQuadColor(FColor C1, FColor C2); // 新增

    void TriangulateWithRiver(EHexDirection Direction, AHexCell* Cell, FVector Center, HexMetrics::FEdgeVertices E);

    void AddTriangleColor(FColor Color);

    void AddQuadColor(FColor Color);

    void TriangulateWithRiverBeginOrEnd(EHexDirection Direction, AHexCell* Cell, FVector Center, HexMetrics::FEdgeVertices E);

    //void TriangulateAdjacentToRiver(EHexDirection Direction, AHexCell* Cell, FVector Center, HexMetrics::FEdgeVertices E);

    void TriangulateEdgeFan(FVector Center, HexMetrics::FEdgeVertices Edge, FColor Color);
    void TriangulateRiverEdgeFan(FVector Center, HexMetrics::FEdgeVertices Edge, FColor Color); // 新增

    void TriangulateEdgeStrip(HexMetrics::FEdgeVertices E1, FColor C1, HexMetrics::FEdgeVertices E2, FColor C2);
    void TriangulateConnection(EHexDirection Direction, AHexCell* Cell, HexMetrics::FEdgeVertices E1);
    void TriangulateEdgeTerraces(HexMetrics::FEdgeVertices Begin, AHexCell* BeginCell, HexMetrics::FEdgeVertices End, AHexCell* EndCell);
    void TriangulateCorner(FVector Bottom, AHexCell* BottomCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell);
    void TriangulateCornerTerraces(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell);
    void TriangulateCornerTerracesCliff(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell);
    void TriangulateCornerCliffTerraces(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell);
    void TriangulateBoundaryTriangle(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Boundary, FLinearColor BoundaryColor);
};