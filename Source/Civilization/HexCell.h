#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/WidgetComponent.h" 
#include "HexCoordinates.h"
#include "HexDirection.h" 
#include "HexMetrics.h"
#include "HexGridChunk.h" 
#include "HexCell.generated.h"

enum class EHexDirection : uint8;

UCLASS()
class CIVILIZATION_API AHexCell : public AActor
{
    GENERATED_BODY()

public:
    AHexCell();

    UPROPERTY()
    AHexGridChunk* Chunk;

    UPROPERTY()
    TArray<FVector> PerturbedCorners; // 存储六边形的 6 个顶点（经过扰动）

    void SetupCell(int32 X, int32 Z, const FVector& Position);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Cell")
    FHexCoordinates Coordinates;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Cell")
    FLinearColor Color;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Cell")
    int32 Elevation = 0;

    UFUNCTION(BlueprintCallable, Category = "Hex Cell")
    int32 GetElevation() const { return Elevation; }

    UFUNCTION(BlueprintCallable, Category = "Hex Cell")
    void SetElevation(int32 NewElevation);

    UFUNCTION(BlueprintCallable, Category = "Hex Cell")
    AHexCell* GetNeighbor(EHexDirection Direction) const;

    UFUNCTION(BlueprintCallable, Category = "Hex Cell")
    void SetNeighbor(EHexDirection Direction, AHexCell* Cell);

    HexMetrics::EHexEdgeType GetEdgeType(EHexDirection Direction);
    HexMetrics::EHexEdgeType GetEdgeType(AHexCell* OtherCell);

    FVector GetPosition() const { return GetActorLocation(); }

    // Road
    UFUNCTION(BlueprintCallable, Category = "Road")
    void RemoveOutgoingRoad();

    UFUNCTION(BlueprintCallable, Category = "Road")
    void RemoveIncomingRoad();

    UFUNCTION(BlueprintCallable, Category = "Road")
    void RemoveRoad();

    UFUNCTION(BlueprintCallable, Category = "Road")
    void SetOutgoingRoad(EHexDirection Direction);

    UFUNCTION(BlueprintCallable, Category = "Road")
    void SetIncomingRoad(EHexDirection Direction);

    UPROPERTY(BlueprintReadOnly, Category = "Road")
    bool bHasIncomingRoad = false;

    UPROPERTY(BlueprintReadOnly, Category = "Road")
    bool bHasOutgoingRoad = false;

    UPROPERTY(BlueprintReadOnly, Category = "Road")
    EHexDirection IncomingRoad = EHexDirection::NE;

    UPROPERTY(BlueprintReadOnly, Category = "Road")
    EHexDirection OutgoingRoad = EHexDirection::NE;

    UFUNCTION(BlueprintCallable, Category = "Road")
    bool HasRoad() const;

    UFUNCTION(BlueprintCallable, Category = "Road")
    bool HasRoadBeginOrEnd() const { return bHasIncomingRoad != bHasOutgoingRoad; }

    UFUNCTION(BlueprintCallable, Category = "Road")
    bool HasRoadThroughEdge(EHexDirection Direction) const;

    UFUNCTION(BlueprintCallable, Category = "Hex Cell")
    void RefreshSelfOnly();

    void SetHighlighted(bool bHighlight);

    UFUNCTION(BlueprintCallable, Category = "HexCell")
    bool IsHighlighted() const { return bIsHighlighted; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexCell|Highlight")
    UMaterialInterface* HighlightMaterial;
private:

    UPROPERTY()
    TArray<AHexCell*> Neighbors;

    UPROPERTY()
    bool bIsHighlighted;
protected:
    UPROPERTY(VisibleAnywhere)
    USceneComponent* SceneRoot;

    // 高亮 Mesh 组件
    UPROPERTY(Transient)
    UProceduralMeshComponent* HighlightMeshComponent;
};