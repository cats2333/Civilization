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

    // river
    void RemoveOutgoingRiver();
    void RemoveIncomingRiver();
    void RemoveRiver();
    void SetOutgoingRiver(EHexDirection Direction);
    
    UPROPERTY(BlueprintReadOnly, Category = "River")
    bool bHasIncomingRiver = false;

    UPROPERTY(BlueprintReadOnly, Category = "River")
    bool bHasOutgoingRiver = false;

    UPROPERTY(BlueprintReadOnly, Category = "River")
    EHexDirection IncomingRiver = EHexDirection::NE;

    UPROPERTY(BlueprintReadOnly, Category = "River")
    EHexDirection OutgoingRiver = EHexDirection::NE;

    UFUNCTION(BlueprintCallable, Category = "River")
    bool HasRiver() const
    {
        bool Result = bHasIncomingRiver || bHasOutgoingRiver;
        UE_LOG(LogTemp, Log, TEXT("Cell (%d, %d) HasRiver: %d"), Coordinates.X, Coordinates.Z, Result);
        return Result;
    }

    UFUNCTION(BlueprintCallable, Category = "River")
    bool HasRiverBeginOrEnd() const { return bHasIncomingRiver != bHasOutgoingRiver; }

    UFUNCTION(BlueprintCallable, Category = "River")
    bool HasRiverThroughEdge(EHexDirection Direction) const
    {
        bool Result = (bHasIncomingRiver && IncomingRiver == Direction) ||
            (bHasOutgoingRiver && OutgoingRiver == Direction);
        UE_LOG(LogTemp, Log, TEXT("Cell (%d, %d) HasRiverThroughEdge in direction %d: %d"),
            Coordinates.X, Coordinates.Z, static_cast<int32>(Direction), Result);
        return Result;
    }

    UFUNCTION(BlueprintCallable, Category = "HexCell|River")
    float GetStreamBedZ() const
    {
        return (Elevation + HexMetrics::StreamBedElevationOffset) * HexMetrics::ElevationStep;
    }

    // 新增刷新方法
    void RefreshSelfOnly();
private:
    UPROPERTY()
    TArray<AHexCell*> Neighbors;
protected:
    UPROPERTY(VisibleAnywhere)
    USceneComponent* SceneRoot;
};