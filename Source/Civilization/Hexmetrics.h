#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Math/Vector.h" 
#include "HexDirection.h"

class UTexture2D;

struct HexMetrics
{
    static const int32 ChunkSizeX = 5;
    static const int32 ChunkSizeZ = 5;
    static const float OuterRadius;
    static const float InnerRadius;
    static const float SolidFactor;
    static const float BlendFactor;
    static const float ElevationStep;
    static const int32 TerracesPerSlope = 2;
    static const int32 TerraceSteps = TerracesPerSlope * 2 + 1;
    static const float HorizontalTerraceStepSize;
    static const float VerticalTerraceStepSize;
    static const float StreamBedElevationOffset;

    static float CellPerturbStrength;
    static float NoiseScale;

    static UTexture2D* NoiseSource;
    static TArray<FColor> NoiseData;

    // 调整后的顶点，模拟顺时针 90 度旋转
    static TArray<FVector> Corners;

    struct FEdgeVertices
    {
        FVector V1, V2, V3, V4, V5;
        FEdgeVertices() {}
        //FEdgeVertices(FVector InV1, FVector InV5, float OuterScale = 0.0f);
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

    enum class EHexEdgeType
    {
        Flat,
        Slope,
        Cliff
    };

    static FVector GetFirstSolidCorner(EHexDirection Direction);
    static FVector GetSecondSolidCorner(EHexDirection Direction);
    static FVector GetFirstCorner(EHexDirection Direction);
    static FVector GetSecondCorner(EHexDirection Direction);
    static FVector GetBridge(EHexDirection Direction);
    static EHexEdgeType GetEdgeType(int32 Elevation1, int32 Elevation2);
    static FVector4 SampleNoise(FVector Position);
    static FVector Perturb(FVector Position);
    static FVector TerraceLerp(FVector A, FVector B, int32 Step);
    static FLinearColor TerraceLerp(FLinearColor A, FLinearColor B, int32 Step);
    static FEdgeVertices TerraceLerp(FEdgeVertices A, FEdgeVertices B, int32 Step);

    static EHexDirection Opposite(EHexDirection Direction);
};

// 初始化 Corners
