#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HexGrid.h"
#include "HexCell.h"
#include "HexMapEditor.generated.h"

UENUM(BlueprintType)
enum class EEditMode : uint8
{
    Color,
    Elevation,
    Road
};

UENUM(BlueprintType)
enum class EEditRoadMode : uint8 
{
    No,
    Yes,
    Ignore
};

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class CIVILIZATION_API UHexMapEditor : public UActorComponent
{
    GENERATED_BODY()

public:
    UHexMapEditor();
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "HexMapEditor")
    void ShowEditorUI(bool bVisible);

    UFUNCTION(BlueprintCallable, Category = "HexMapEditor")
    void SetBrushSize(float Size);

    UFUNCTION(BlueprintCallable, Category = "HexMapEditor")
    void SetElevation(float Elevation);

    UFUNCTION(BlueprintCallable, Category = "HexMapEditor")
    void SelectColor(int32 Index);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    AHexGrid* HexGrid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    TArray<FLinearColor> Colors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    EEditMode EditMode = EEditMode::Color;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    EEditRoadMode RoadMode = EEditRoadMode::No;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    float MoveSpeed = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    float SwivelMinZoom = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    float SwivelMaxZoom = 80.0f;

    UFUNCTION(BlueprintCallable, Category = "HexMapEditor")
    TArray<FString> GetEditModeOptions();

    UFUNCTION(BlueprintCallable, Category = "HexMapEditor")
    TArray<FString> GetRoadModeOptions();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HexMapEditor")
    int32 BrushSize = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HexMapEditor")
    int32 ActiveElevation = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HexMapEditor")
    FLinearColor ActiveColor;

    UPROPERTY(Transient)
    AHexCell* PreviousCell;

    UPROPERTY()
    bool bIsFirstClick;

    void HandleInput();
    void EditCells(AHexCell* Center);
    void EditCell(AHexCell* Cell);

    void ClearHighlight();

    void MoveCameraForward(float AxisValue);
    void MoveCameraRight(float AxisValue);
    void MoveCamera(float AxisValue, bool bIsForward);
    void AdjustCameraZoom(float Value);
    void RotateCamera(float AxisValue);
};