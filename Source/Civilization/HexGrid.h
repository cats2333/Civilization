#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexMetrics.h"
#include "HexDirection.h"
#include "Math/Vector.h" 
#include "HexGrid.generated.h"

class AHexCell;
class AHexGridChunk;
struct FHexCoordinates;

UCLASS()
class CIVILIZATION_API AHexGrid : public AActor
{
    GENERATED_BODY()

public:
    AHexGrid();

    virtual void BeginPlay() override;

    void CreateChunks();
    void CreateCells();
    void AddCellToChunk(int32 X, int32 Z, AHexCell* Cell);
    void TriangulateCells();

    AHexCell* GetCellByPosition(FVector Position);
    AHexCell* GetCellByCoordinates(FHexCoordinates Coordinates);

    UFUNCTION(BlueprintCallable, Category = "HexGrid")
    void Refresh();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexGrid")
    TSubclassOf<AHexCell> CellClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexGrid")
    TSubclassOf<AHexGridChunk> ChunkClass;

    // ÐÂÔö Getter
    UFUNCTION(BlueprintCallable, Category = "HexGrid")
    TArray<AHexCell*> GetCells() const { return Cells; }
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexGrid")
    int32 ChunkCountX = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexGrid")
    int32 ChunkCountZ = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexGrid")
    UTexture2D* NoiseSource;

    UPROPERTY()
    TArray<AHexCell*> Cells;

    UPROPERTY()
    TArray<AHexGridChunk*> Chunks;

    int32 CellCountX;
    int32 CellCountZ;
    int32 Width;
    int32 Height;
};