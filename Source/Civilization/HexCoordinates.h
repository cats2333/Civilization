#pragma once

#include "CoreMinimal.h"
#include "HexCoordinates.generated.h"

USTRUCT(BlueprintType)
struct CIVILIZATION_API FHexCoordinates
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 X;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Z;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Y;

    FHexCoordinates(int32 InX = 0, int32 InZ = 0);

    static FHexCoordinates FromOffsetCoordinates(int32 OffsetX, int32 OffsetZ);
    static FHexCoordinates FromPosition(FVector Position);
    FString ToString() const;
    FString ToStringOnSeparateLines() const;
};