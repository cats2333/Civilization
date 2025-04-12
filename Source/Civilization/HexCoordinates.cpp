#include "HexCoordinates.h"
#include "HexMetrics.h"

FHexCoordinates FHexCoordinates::FromPosition(FVector Position)
{
    const float SpacingFactor = 1.0f;
    float X = (Position.X / SpacingFactor) / (HexMetrics::InnerRadius * 2.0f);
    float Y = -X;

    float Offset = Position.Y / (HexMetrics::OuterRadius * 3.0f);
    X -= Offset;
    Y -= Offset;

    UE_LOG(LogTemp, Log, TEXT("FromPosition: Position=(%s), Raw X=%f, Y=%f, Offset=%f"), *Position.ToString(), X, Y, Offset);

    int32 iX = FMath::RoundToInt(X);
    int32 iY = FMath::RoundToInt(Y);
    int32 iZ = FMath::RoundToInt(-X - Y);

    if (iX + iY + iZ != 0)
    {
        float dX = FMath::Abs(X - iX);
        float dY = FMath::Abs(Y - iY);
        float dZ = FMath::Abs(-X - Y - iZ);

        UE_LOG(LogTemp, Log, TEXT("Rounding deltas: dX=%f, dY=%f, dZ=%f"), dX, dY, dZ);

        if (dX > dY && dX > dZ)
        {
            iX = -iY - iZ;
        }
        else if (dZ > dY)
        {
            iZ = -iX - iY;
        }

        UE_LOG(LogTemp, Log, TEXT("Adjusted coordinates: (%d, %d, %d)"), iX, iY, iZ);
    }

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