#include "HexCoordinates.h"
#include "HexMetrics.h"

FHexCoordinates FHexCoordinates::FromPosition(FVector Position)
{
    const float SpacingFactor = 1.0f;

    float OldPosX = Position.X;
    float OldPosY = Position.Y;

    float Z = (OldPosY / (HexMetrics::OuterRadius * 1.5f * SpacingFactor)) + (15 - 1) / 2.0f;
    float TempX = OldPosX / (HexMetrics::InnerRadius * 2.0f * SpacingFactor);
    float X = TempX - (Z * 0.5f - Z / 2);

    int32 iX = FMath::RoundToInt(X);
    int32 iZ = FMath::RoundToInt(Z);
    int32 iY = -iX - iZ;

    UE_LOG(LogTemp, Log, TEXT("FromPosition: Position=(%s), Old=(%f, %f), X=%f, Z=%f, Coordinates=(%d, %d, %d)"),
        *Position.ToString(), OldPosX, OldPosY, X, Z, iX, iY, iZ);

    return FHexCoordinates(iX, iZ);
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