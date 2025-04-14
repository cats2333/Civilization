#include "HexMetrics.h"
#include "Engine/Texture2D.h"

const float HexMetrics::OuterRadius = 1.0f;
const float HexMetrics::InnerRadius = OuterRadius * FMath::Sqrt(3.0f) / 2.0f;
const float HexMetrics::SolidFactor = 0.75f;
const float HexMetrics::BlendFactor = 1.0f - SolidFactor;
const float HexMetrics::ElevationStep = 1.0f;
const float HexMetrics::HorizontalTerraceStepSize = 1.0f / TerraceSteps;
const float HexMetrics::VerticalTerraceStepSize = 1.0f / (TerracesPerSlope + 1);
const float HexMetrics::StreamBedElevationOffset = 0.f;//-1

float HexMetrics::CellPerturbStrength = 0.5f; // 1.5 is normal
float HexMetrics::NoiseScale = 0.01f; // 0.01 normal

UTexture2D* HexMetrics::NoiseSource = nullptr;TArray<FColor> HexMetrics::NoiseData;TArray<FVector> HexMetrics::Corners = {
    FVector(0.0f, OuterRadius, 0.0f),           // NE
    FVector(InnerRadius, OuterRadius / 2, 0.0f), // E
    FVector(InnerRadius, -OuterRadius / 2, 0.0f), // SE
    FVector(0.0f, -OuterRadius, 0.0f),          // SW
    FVector(-InnerRadius, -OuterRadius / 2, 0.0f), // W
    FVector(-InnerRadius, OuterRadius / 2, 0.0f)  // NW
};
FVector HexMetrics::GetBridge(EHexDirection Direction)
{
    return (Corners[static_cast<int32>(Direction)] + Corners[(static_cast<int32>(Direction) + 1) % 6]) * BlendFactor;

}
HexMetrics::EHexEdgeType HexMetrics::GetEdgeType(int32 Elevation1, int32 Elevation2)
{
    if (Elevation1 == Elevation2)
    {
        return EHexEdgeType::Flat;
    }
    int32 Delta = Elevation2 - Elevation1;
    if (Delta == 1 || Delta == -1)
    {
        return EHexEdgeType::Slope;
    }
    return EHexEdgeType::Cliff;

}FVector4 HexMetrics::SampleNoise(FVector Position)
{
    if (NoiseData.Num() == 0 || !NoiseSource)
    {
        return FVector4(0.5f, 0.5f, 0.5f, 0.5f);
    }

    int32 Width = NoiseSource->GetSizeX();
    int32 Height = NoiseSource->GetSizeY();

    float U = (Position.X * NoiseScale) - FMath::FloorToFloat(Position.X * NoiseScale);
    float V = (Position.Y * NoiseScale) - FMath::FloorToFloat(Position.Y * NoiseScale);
    U = FMath::Clamp(U, 0.0f, 1.0f);
    V = FMath::Clamp(V, 0.0f, 1.0f);

    int32 X0 = FMath::FloorToInt(U * (Width - 1));
    int32 Y0 = FMath::FloorToInt(V * (Height - 1));
    int32 X1 = FMath::Min(X0 + 1, Width - 1);
    int32 Y1 = FMath::Min(Y0 + 1, Height - 1);

    float FracX = U * (Width - 1) - X0;
    float FracY = V * (Height - 1) - Y0;

    FColor C00 = NoiseData[Y0 * Width + X0];
    FColor C10 = NoiseData[Y0 * Width + X1];
    FColor C01 = NoiseData[Y1 * Width + X0];
    FColor C11 = NoiseData[Y1 * Width + X1];

    FVector4 Sample;
    Sample.X = FMath::Lerp(FMath::Lerp((float)C00.R / 255.0f, (float)C10.R / 255.0f, FracX), FMath::Lerp((float)C01.R / 255.0f, (float)C11.R / 255.0f, FracX), FracY);
    Sample.Y = FMath::Lerp(FMath::Lerp((float)C00.G / 255.0f, (float)C10.G / 255.0f, FracX), FMath::Lerp((float)C01.G / 255.0f, (float)C11.G / 255.0f, FracX), FracY);
    Sample.Z = FMath::Lerp(FMath::Lerp((float)C00.B / 255.0f, (float)C10.B / 255.0f, FracX), FMath::Lerp((float)C01.B / 255.0f, (float)C11.B / 255.0f, FracX), FracY);
    Sample.W = FMath::Lerp(FMath::Lerp((float)C00.A / 255.0f, (float)C10.A / 255.0f, FracX), FMath::Lerp((float)C01.A / 255.0f, (float)C11.A / 255.0f, FracX), FracY);

    return Sample;

}FVector HexMetrics::Perturb(FVector Position)
{
    FVector4 Sample = SampleNoise(Position);
    Position.X += (Sample.X * 2.0f - 1.0f) * CellPerturbStrength;
    Position.Y += (Sample.Y * 2.0f - 1.0f) * CellPerturbStrength;
    return Position;

}

FVector HexMetrics::TerraceLerp(FVector A, FVector B, int32 Step)
{
    float H = Step * HorizontalTerraceStepSize;
    A.X += (B.X - A.X) * H;
    A.Y += (B.Y - A.Y) * H; // Unreal 中 Y 是水平方向
    float V = ((Step + 1) / 2) * VerticalTerraceStepSize; // 奇数步调整高度
    A.Z += (B.Z - A.Z) * V;
    return A;
}
FLinearColor HexMetrics::TerraceLerp(FLinearColor A, FLinearColor B, int32 Step)
{
    float H = Step * HorizontalTerraceStepSize;
    return FLinearColor::LerpUsingHSV(A, B, H);
}

HexMetrics::FEdgeVertices HexMetrics::TerraceLerp(FEdgeVertices A, FEdgeVertices B, int32 Step)
{
    FEdgeVertices Result;
    Result.V1 = TerraceLerp(A.V1, B.V1, Step);
    Result.V2 = TerraceLerp(A.V2, B.V2, Step);
    Result.V3 = TerraceLerp(A.V3, B.V3, Step);
    Result.V4 = TerraceLerp(A.V4, B.V4, Step);
    Result.V5 = TerraceLerp(A.V5, B.V5, Step);
    return Result;
}


