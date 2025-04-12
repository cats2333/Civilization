#pragma once
#include "CoreMinimal.h"
#include "HexDirection.h"

class HexMetrics
{
public:
    static TArray<FColor> NoiseData;

    enum class EHexEdgeType
    {
        Flat,
        Slope,
        Cliff
    };

    static EHexEdgeType GetEdgeType(int32 Elevation1, int32 Elevation2);

    struct FEdgeVertices
    {
        FVector V1, V2, V3, V4, V5;

        FEdgeVertices() : V1(0, 0, 0), V2(0, 0, 0), V3(0, 0, 0), V4(0, 0, 0), V5(0, 0, 0) {}

        FEdgeVertices(FVector Corner1, FVector Corner2)
        {
            V1 = Corner1;
            V2 = FMath::Lerp(Corner1, Corner2, 0.25f);
            V3 = FMath::Lerp(Corner1, Corner2, 0.5f);
            V4 = FMath::Lerp(Corner1, Corner2, 0.75f);
            V5 = Corner2;
        }

        FEdgeVertices(FVector Corner1, FVector Corner2, float OuterStep)
        {
            V1 = Corner1;
            V2 = FMath::Lerp(Corner1, Corner2, OuterStep);
            V3 = FMath::Lerp(Corner1, Corner2, 0.5f);
            V4 = FMath::Lerp(Corner1, Corner2, 1.0f - OuterStep);
            V5 = Corner2;
        }
    };


    static const float OuterRadius;
    static const float InnerRadius;

    static const int32 ChunkSizeX = 5;
    static const int32 ChunkSizeZ = 5;

    static const float SolidFactor;
    static const float BlendFactor;

    static const float ElevationStep;
    static const float StreamBedElevationOffset;

    static const int32 TerracesPerSlope = 2;
    static const int32 TerraceSteps = TerracesPerSlope * 2 + 1;
    static const float HorizontalTerraceStepSize;
    static const float VerticalTerraceStepSize;

    static TArray<FVector> Corners;

    static UTexture2D* NoiseSource;
    static float CellPerturbStrength;
    static float NoiseScale;

    static FVector4 SampleNoise(FVector Position);
    static FVector Perturb(FVector Position);

    static EHexDirection Opposite(EHexDirection Direction)
    {
        int32 DirIndex = static_cast<int32>(Direction);
        return DirIndex < 3 ? static_cast<EHexDirection>(DirIndex + 3) : static_cast<EHexDirection>(DirIndex - 3);
    }

    static EHexDirection Next(EHexDirection Direction)
    {
        return static_cast<EHexDirection>((static_cast<int32>(Direction) + 1) % 6);
    }

    static EHexDirection Previous(EHexDirection Direction)
    {
        return static_cast<EHexDirection>((static_cast<int32>(Direction) - 1 + 6) % 6);
    }

    static FVector GetFirstCorner(EHexDirection Direction) { return Corners[static_cast<int32>(Direction)]; }
    static FVector GetSecondCorner(EHexDirection Direction) { return Corners[(static_cast<int32>(Direction) + 1) % 6]; }
    static FVector GetFirstSolidCorner(EHexDirection Direction) { return Corners[static_cast<int32>(Direction)] * SolidFactor; }
    static FVector GetSecondSolidCorner(EHexDirection Direction) { return Corners[(static_cast<int32>(Direction) + 1) % 6] * SolidFactor; }
    static FVector GetBridge(EHexDirection Direction);

    static FVector TerraceLerp(FVector A, FVector B, int32 Step);
    static FLinearColor TerraceLerp(FLinearColor A, FLinearColor B, int32 Step);
    static FEdgeVertices TerraceLerp(FEdgeVertices A, FEdgeVertices B, int32 Step);
};