#include "HexCoordinates.h"
#include "HexMetrics.h"

FHexCoordinates FHexCoordinates::FromPosition(FVector Position)
{
    const float SpacingFactor = 1.0f;
    float X = (Position.X / SpacingFactor) / (HexMetrics::InnerRadius * 2.0f);
    float Z = (Position.Y / SpacingFactor) / (HexMetrics::OuterRadius * 1.5f);

    int32 iX = FMath::RoundToInt(X);
    float AdjustedY = Position.Y - ((iX % 2 == 0) ? 0.0f : (HexMetrics::OuterRadius * 1.5f / 2.0f));
    Z = (AdjustedY / SpacingFactor) / (HexMetrics::OuterRadius * 1.5f);

    int32 iZ = FMath::RoundToInt(Z);

    UE_LOG(LogTemp, Log, TEXT("FromPosition: Position=(%s), X=%f, Z=%f, iX=%d, iZ=%d"),
        *Position.ToString(), X, Z, iX, iZ);

    return FHexCoordinates::FromOffsetCoordinates(iX, iZ);
}

FHexCoordinates::FHexCoordinates(int32 InX, int32 InZ)
    : X(InX), Z(InZ), Y(-InX - InZ)
{
    UE_LOG(LogTemp, Log, TEXT("Constructed FHexCoordinates: (%d, %d, %d)"), X, Y, Z);
}

FHexCoordinates FHexCoordinates::FromOffsetCoordinates(int32 OffsetX, int32 OffsetZ)
{
    int32 NewX = OffsetX - OffsetZ / 2;
    int32 NewZ = OffsetZ;
    FHexCoordinates Coords(NewX, NewZ);
    UE_LOG(LogTemp, Log, TEXT("FromOffsetCoordinates: (%d, %d) -> (%d, %d, %d)"), OffsetX, OffsetZ, Coords.X, Coords.Y, Coords.Z);
    return Coords;
}

FString FHexCoordinates::ToString() const
{
    return FString::Printf(TEXT("(%d, %d, %d)"), X, Y, Z);
}

FString FHexCoordinates::ToStringOnSeparateLines() const
{
    return FString::Printf(TEXT("%d\n%d\n%d"), X, Y, Z);
}