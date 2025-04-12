#pragma once

#include "CoreMinimal.h"
#include "HexDirection.generated.h"

UENUM(BlueprintType)
enum class EHexDirection : uint8
{
    NE UMETA(DisplayName = "Northeast"),
    E  UMETA(DisplayName = "East"),
    SE UMETA(DisplayName = "Southeast"),
    SW UMETA(DisplayName = "Southwest"),
    W  UMETA(DisplayName = "West"),
    NW UMETA(DisplayName = "Northwest")
};

static EHexDirection Next(EHexDirection Direction)
{
    return static_cast<EHexDirection>((static_cast<int32>(Direction) + 1) % 6);
}

static EHexDirection Previous(EHexDirection Direction)
{
    return static_cast<EHexDirection>((static_cast<int32>(Direction) + 5) % 6);
}