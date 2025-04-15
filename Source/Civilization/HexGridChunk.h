#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexMetrics.h"
#include "ProceduralMeshComponent.h"
#include "HexGridChunk.generated.h"


class AHexCell;
class UProceduralMeshComponent;
class UDecalComponent;

UCLASS()
class CIVILIZATION_API AHexGridChunk : public AActor
{
    GENERATED_BODY()

public:
    AHexGridChunk();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
   
    void AddCell(int32 Index, AHexCell* Cell);
    void ClearCells();
    void Refresh();
    void TriangulateCells();

    UPROPERTY()
    TArray<AHexCell*> Cells;

    // 贴花相关函数
    void ClearRoadDecals();
    void CreateRoadDecal(FVector Start, FVector End, float Width, UMaterialInterface* DecalMaterial);

    UMaterialInterface* RoadDecalMaterial;
protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UProceduralMeshComponent* HexMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TArray<UDecalComponent*> RoadDecals;



    // 地形网格数据
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FColor> VertexColors;
    TArray<FVector2D> UV0;
    TArray<FProcMeshTangent> Tangents;


    void AddTriangle(FVector V1, FVector V2, FVector V3);
    void AddTriangleColor(FColor C1, FColor C2, FColor C3);
    void AddTriangleColor(FColor Color);
    void AddQuad(FVector V1, FVector V2, FVector V3, FVector V4);
    void AddQuadColor(FColor C1, FColor C2, FColor C3, FColor C4);
    void AddQuadColor(FColor C1, FColor C2);
    void AddQuadColor(FColor Color);
    void TriangulateEdgeFan(FVector Center, HexMetrics::FEdgeVertices Edge, FColor Color);
    void TriangulateEdgeStrip(HexMetrics::FEdgeVertices E1, FColor C1, HexMetrics::FEdgeVertices E2, FColor C2);

    void TriangulateConnection(EHexDirection Direction, AHexCell* Cell, HexMetrics::FEdgeVertices E1);
    void TriangulateEdgeTerraces(HexMetrics::FEdgeVertices Begin, AHexCell* BeginCell, HexMetrics::FEdgeVertices End, AHexCell* EndCell);
    void TriangulateCorner(FVector Bottom, AHexCell* BottomCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell);
    void TriangulateCornerTerraces(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell);
    void TriangulateCornerTerracesCliff(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell);
    void TriangulateCornerCliffTerraces(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Right, AHexCell* RightCell);
    void TriangulateBoundaryTriangle(FVector Begin, AHexCell* BeginCell, FVector Left, AHexCell* LeftCell, FVector Boundary, FLinearColor BoundaryColor);
    void AddTriangleUnperturbed(FVector V1, FVector V2, FVector V3);

    
    UPROPERTY()
    UMaterialInterface* DefaultMaterial;

    UPROPERTY()
    UMaterialInterface* HighlightMaterial;

    void ClearMeshData();
    void ApplyMesh();
};