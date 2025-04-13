#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/WidgetComponent.h" 
#include "HexCoordinates.h"
#include "HexDirection.h" 
#include "HexMetrics.h"
#include "HexGridChunk.h" //
#include "HexCell.generated.h"

// 前向声明 EHexDirection
enum class EHexDirection : uint8;

UCLASS()
class CIVILIZATION_API AHexCell : public AActor
{
    GENERATED_BODY()

public:
    AHexCell();

    UPROPERTY()
    AHexGridChunk* Chunk;

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

    // 刷新方法
    UFUNCTION(BlueprintCallable, Category = "Hex Cell")
    void RefreshSelfOnly();
private:
    UPROPERTY()
    TArray<AHexCell*> Neighbors;
protected:
    UPROPERTY(VisibleAnywhere)
    USceneComponent* SceneRoot;
};