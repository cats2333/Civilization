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
    River
};

UENUM(BlueprintType)
enum class EEditRiverMode : uint8
{
    Ignore,
    Yes,
    No
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
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

    UFUNCTION(BlueprintCallable, Category = "HexMapEditor")
    void SetRiverMode(int32 Mode) { RiverMode = static_cast<EEditRiverMode>(Mode); }

    //  ‰»Î∞Û∂®
    UFUNCTION()
    void MoveCameraForward(float AxisValue);

    UFUNCTION()
    void MoveCameraRight(float AxisValue);

    UFUNCTION()
    void AdjustCameraZoom(float AxisValue);

    UFUNCTION()
    void RotateCamera(float AxisValue);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    AHexGrid* HexGrid;

    UFUNCTION(BlueprintCallable, Category = "HexMapEditor")
    TArray<FString> GetEditModeOptions();

    UFUNCTION(BlueprintCallable, Category = "HexMapEditor")
    TArray<FString> GetRiverModeOptions();

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    TArray<FLinearColor> Colors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    FLinearColor ActiveColor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    int32 ActiveElevation = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    EEditMode EditMode = EEditMode::Color;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    int32 BrushSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    float MoveSpeed = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    float SwivelMinZoom = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    float SwivelMaxZoom = 80.0f;

    UPROPERTY()
    class UUserWidget* HexMapEditorUI;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor|River")
    EEditRiverMode RiverMode = EEditRiverMode::Ignore;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HexMapEditor|River")
    bool bIsDragging = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HexMapEditor|River")
    EHexDirection DragDirection = EHexDirection::NE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexMapEditor")
    AHexCell* PreviousCell = nullptr;

private:
    void HandleInput();
    void EditCells(AHexCell* Center);
    void EditCell(AHexCell* Cell);
    void ValidateDrag(AHexCell* CurrentCell);
    void MoveCamera(float AxisValue, bool bIsForward);
};

