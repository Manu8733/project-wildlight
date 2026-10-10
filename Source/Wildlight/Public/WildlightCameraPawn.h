#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "WildlightCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

/** Strategic camera whose world-space interface does not depend on the simulation. */
UCLASS()
class WILDLIGHT_API AWildlightCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    AWildlightCameraPawn();
    virtual void Tick(float DeltaSeconds) override;

    /** Supply the current forward/right axes each controller tick. */
    UFUNCTION(BlueprintCallable, Category = "Wildlight|Camera")
    void MoveCamera(float ForwardInput, float RightInput);

    /** Supply the current rotation axis each controller tick. Positive rotates clockwise. */
    UFUNCTION(BlueprintCallable, Category = "Wildlight|Camera")
    void RotateCamera(float Input);

    /** Positive wheel input zooms in. */
    UFUNCTION(BlueprintCallable, Category = "Wildlight|Camera")
    void ZoomCamera(float Input);

    UFUNCTION(BlueprintCallable, Category = "Wildlight|Camera")
    void FocusAt(FVector WorldLocation);

    /** Set positive map width/depth. The map origin is (0, 0). */
    UFUNCTION(BlueprintCallable, Category = "Wildlight|Camera")
    void SetWorldBounds(FVector2D Bounds);

    /** Project the cursor onto the ground plane, without depending on collision actors. */
    UFUNCTION(BlueprintPure, Category = "Wildlight|Camera")
    bool GetGroundTarget(FVector& OutLocation) const;

    /** The controller enables this after starting a game and disables it over menus. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera")
    bool bCameraInputEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera")
    bool bEdgeScrollingEnabled = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wildlight|Camera")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wildlight|Camera")
    TObjectPtr<USpringArmComponent> CameraArm;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera", meta = (ClampMin = "1.0"))
    float PanSpeed = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera", meta = (ClampMin = "1.0"))
    float RotationSpeed = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera", meta = (ClampMin = "1.0"))
    float ZoomStep = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera", meta = (ClampMin = "1.0"))
    float MinimumZoom = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera", meta = (ClampMin = "1.0"))
    float MaximumZoom = 11000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera", meta = (ClampMin = "0.0"))
    float ZoomSmoothing = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera", meta = (ClampMin = "0.0"))
    float PanSmoothing = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera", meta = (ClampMin = "0.0"))
    float RotationSmoothing = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera", meta = (ClampMin = "1.0"))
    float EdgeScrollPixels = 24.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wildlight|Camera")
    float GroundHeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wildlight|Camera")
    FVector2D WorldBounds = FVector2D(12800.0f, 9600.0f);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Wildlight|Camera")
    TObjectPtr<USceneComponent> SceneRoot;

    FVector2D MovementInput = FVector2D::ZeroVector;
    float RotationInput = 0.0f;
    float TargetYaw = 45.0f;
    float TargetZoom = 6000.0f;
    FVector PanVelocity = FVector::ZeroVector;

    FVector2D ReadEdgeScroll() const;
    FVector ClampToWorld(FVector Location) const;
};
