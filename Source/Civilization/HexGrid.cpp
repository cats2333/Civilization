#include "HexGrid.h"
#include "HexCell.h"
#include "HexMetrics.h"
#include "HexCoordinates.h"
#include "HexGridChunk.h"

AHexGrid::AHexGrid()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AHexGrid::BeginPlay()
{
    Super::BeginPlay();
    HexMetrics::NoiseSource = NoiseSource;

    CellCountX = ChunkCountX * HexMetrics::ChunkSizeX;
    CellCountZ = ChunkCountZ * HexMetrics::ChunkSizeZ;
    Width = CellCountX;
    Height = CellCountZ;
    UE_LOG(LogTemp, Log, TEXT("CellCountX: %d, CellCountZ: %d, Width: %d, Height: %d"),
        CellCountX, CellCountZ, Width, Height);

    // 初始化 NoiseData
    if (NoiseSource && NoiseSource->GetPlatformData())
    {
        FTexturePlatformData* PlatformData = NoiseSource->GetPlatformData();
        FTexture2DMipMap& Mip = PlatformData->Mips[0];
        FByteBulkData& BulkData = Mip.BulkData;
        uint8* Data = (uint8*)BulkData.LockReadOnly();

        int32 TexWidth = Mip.SizeX;
        int32 TexHeight = Mip.SizeY;
        HexMetrics::NoiseData.SetNum(TexWidth * TexHeight);

        for (int32 Y = 0; Y < TexHeight; Y++)
        {
            for (int32 X = 0; X < TexWidth; X++)
            {
                int32 Index = Y * TexWidth + X;
                HexMetrics::NoiseData[Index] = FColor(
                    Data[Index * 4], Data[Index * 4 + 1], Data[Index * 4 + 2], Data[Index * 4 + 3]
                );
            }
        }
        BulkData.Unlock();
    }

    CreateChunks();
    CreateCells();

    TriangulateCells();
}

void AHexGrid::CreateCells()
{
    if (!CellClass)
    {
        UE_LOG(LogTemp, Error, TEXT("CellClass is not set!"));
        return;
    }

    // 清空旧的 Chunks 和 Cells
    for (AHexGridChunk* Chunk : Chunks)
    {
        if (Chunk && IsValid(Chunk))
        {
            Chunk->ClearCells();
            Chunk->Destroy();
        }
    }
    Chunks.Empty();

    for (AHexCell* Cell : Cells)
    {
        if (Cell && IsValid(Cell))
        {
            Cell->Destroy();
        }
    }
    Cells.Empty();
    Cells.SetNum(Width * Height);

    const float SpacingFactor = 1.0f;

    // 使用类成员 ChunkCountX 和 ChunkCountZ（如果存在）
    ChunkCountX = (Width + HexMetrics::ChunkSizeX - 1) / HexMetrics::ChunkSizeX;
    ChunkCountZ = (Height + HexMetrics::ChunkSizeZ - 1) / HexMetrics::ChunkSizeZ;
    Chunks.SetNum(ChunkCountX * ChunkCountZ);

    for (int32 Z = 0, I = 0; Z < Height; Z++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            float PosX = X * (HexMetrics::InnerRadius * 2.0f) * SpacingFactor;
            float Offset = (X % 2 == 0) ? 0.0f : (HexMetrics::OuterRadius * 1.5f / 2.0f);
            float PosY = (Z * (HexMetrics::OuterRadius * 1.5f) + Offset) * SpacingFactor;
            FVector Position(PosX, PosY, 0.0f);

            AHexCell* Cell = GetWorld()->SpawnActor<AHexCell>(CellClass, Position, FRotator::ZeroRotator);
            if (Cell)
            {
                Cell->SetupCell(X, Z, Position);
                Cell->SetElevation(0);
                Cells[I] = Cell;

                if (X > 0)
                {
                    Cell->SetNeighbor(EHexDirection::W, Cells[I - 1]);
                }

                if (Z > 0)
                {
                    if ((X % 2) == 0)
                    {
                        Cell->SetNeighbor(EHexDirection::SW, Cells[I - Width]);
                        if (X > 0)
                        {
                            Cell->SetNeighbor(EHexDirection::NW, Cells[I - Width - 1]);
                        }
                    }
                    else
                    {
                        Cell->SetNeighbor(EHexDirection::NW, Cells[I - Width]);
                        if (X < Width - 1)
                        {
                            Cell->SetNeighbor(EHexDirection::SW, Cells[I - Width + 1]);
                        }
                    }
                }

                AddCellToChunk(X, Z, Cell);
                I++;
                UE_LOG(LogTemp, Log, TEXT("Spawned HexCell at (%d, %d), Position=%s"), X, Z, *Position.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn HexCell at (%d, %d). CellClass: %s"), X, Z, *GetNameSafe(CellClass));
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Total Cells: %d, Total Chunks: %d"), Cells.Num(), Chunks.Num());
}

void AHexGrid::TriangulateCells()
{
    for (AHexGridChunk* Chunk : Chunks)
    {
        if (Chunk)
        {
            Chunk->TriangulateCells();
        }
    }
}

AHexCell* AHexGrid::GetCellByPosition(FVector Position)
{
    FVector LocalPosition = GetActorTransform().InverseTransformPosition(Position);
    FHexCoordinates Coordinates = FHexCoordinates::FromPosition(LocalPosition);
    UE_LOG(LogTemp, Log, TEXT("Touched at (%d, %d, %d)"), Coordinates.X, Coordinates.Y, Coordinates.Z);

    int32 OffsetX = Coordinates.X;
    int32 OffsetZ = Coordinates.Z;

    if (OffsetX < 0 || OffsetX >= Width || OffsetZ < 0 || OffsetZ >= Height)
    {
        UE_LOG(LogTemp, Warning, TEXT("Coordinates (%d, %d) out of bounds!"), OffsetX, OffsetZ);
        return nullptr;
    }

    int32 Index = OffsetX + OffsetZ * Width;
    if (Index >= 0 && Index < Cells.Num() && Cells[Index])
    {
        UE_LOG(LogTemp, Log, TEXT("Found cell at index %d, coordinates (%d, %d)"), Index, OffsetX, OffsetZ);
        return Cells[Index];
    }

    UE_LOG(LogTemp, Warning, TEXT("No cell found at coordinates (%d, %d)"), Coordinates.X, Coordinates.Z);
    return nullptr;
}

AHexCell* AHexGrid::GetCellByCoordinates(FHexCoordinates Coordinates)
{
    int32 Z = Coordinates.Z;
    if (Z < 0 || Z >= CellCountZ)
    {
        return nullptr;
    }

    int32 X = Coordinates.X + Z / 2;
    if (X < 0 || X >= CellCountX)
    {
        return nullptr;
    }

    int32 Index = X + Z * CellCountX;
    return Cells[Index];
}

void AHexGrid::Refresh()
{
    TriangulateCells();
}

void AHexGrid::CreateChunks()
{
    if (!ChunkClass)
    {
        UE_LOG(LogTemp, Error, TEXT("ChunkClass is not set! Cannot spawn HexGridChunk."));
        return;
    }

    Chunks.Empty();
    Chunks.SetNum(ChunkCountX * ChunkCountZ);

    for (int32 Z = 0, I = 0; Z < ChunkCountZ; Z++)
    {
        for (int32 X = 0; X < ChunkCountX; X++)
        {
            AHexGridChunk* Chunk = GetWorld()->SpawnActor<AHexGridChunk>(ChunkClass, FVector::ZeroVector, FRotator::ZeroRotator);
            if (Chunk)
            {
                Chunk->SetActorTransform(GetActorTransform());
                Chunks[I++] = Chunk;
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn HexGridChunk at (%d, %d)"), X, Z);
            }
        }
    }
}

void AHexGrid::AddCellToChunk(int32 X, int32 Z, AHexCell* Cell)
{
    int32 ChunkX = X / HexMetrics::ChunkSizeX;
    int32 ChunkZ = Z / HexMetrics::ChunkSizeZ;
    int32 ChunkIndex = ChunkX + ChunkZ * ((Width + HexMetrics::ChunkSizeX - 1) / HexMetrics::ChunkSizeX);

    AHexGridChunk* Chunk = Chunks[ChunkIndex];
    if (!Chunk)
    {
        Chunk = GetWorld()->SpawnActor<AHexGridChunk>(ChunkClass, FVector::ZeroVector, FRotator::ZeroRotator);
        if (Chunk)
        {
            Chunk->Cells.SetNum(HexMetrics::ChunkSizeX * HexMetrics::ChunkSizeZ);
            Chunks[ChunkIndex] = Chunk;
            UE_LOG(LogTemp, Log, TEXT("Created new chunk at index %d"), ChunkIndex);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to spawn HexGridChunk at index %d"), ChunkIndex);
            return;
        }
    }

    int32 LocalX = X % HexMetrics::ChunkSizeX;
    int32 LocalZ = Z % HexMetrics::ChunkSizeZ;
    int32 LocalIndex = LocalX + LocalZ * HexMetrics::ChunkSizeX;
    Chunk->AddCell(LocalIndex, Cell);
}