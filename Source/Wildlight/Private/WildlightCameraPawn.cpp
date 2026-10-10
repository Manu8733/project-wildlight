#include "WildlightCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Math/RotationMatrix.h"

AWildlightCameraPawn::AWildlightCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessAI = EAutoPossessAI::Disabled;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(SceneRoot);
    CameraArm->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));
    CameraArm->TargetArmLength = TargetZoom;
    CameraArm->bDoCollisionTest = false;
    CameraArm->bUsePawnControlRotation = false;
    CameraArm->bEnableCameraLag = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
    Camera->ProjectionMode = ECameraProjectionMode::Perspective;
    Camera->FieldOfView = 45.0f;

    SetActorRotation(FRotator(0.0f, TargetYaw, 0.0f));
}

void AWildlightCameraPawn::BeginPlay()
{
    Super::BeginPlay();
    // SpawnActor may supply a zero rotation, so establish the strategic heading here.
    SetActorRotation(FRotator(0.0f, TargetYaw, 0.0f));
    TargetZoom = FMath::Clamp(CameraArm->TargetArmLength, MinimumZoom, FMath::Max(MinimumZoom, MaximumZoom));
    SetActorLocation(ClampToWorld(GetActorLocation()));
}

void AWildlightCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (DeltaSeconds <= 0.0f)
    {
        return;
    }

    if (!bCameraInputEnabled)
    {
        MovementInput = FVector2D::ZeroVector;
        RotationInput = 0.0f;
        PanVelocity = FVector::ZeroVector;
    }

    TargetYaw = FRotator::NormalizeAxis(TargetYaw + RotationInput * RotationSpeed * DeltaSeconds);
    const FRotator DesiredRotation(0.0f, TargetYaw, 0.0f);
    const FRotator NewRotation = RotationSmoothing > 0.0f
        ? FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, RotationSmoothing)
        : DesiredRotation;
    SetActorRotation(NewRotation);

    FVector2D EffectiveInput = MovementInput;
    if (bCameraInputEnabled && bEdgeScrollingEnabled)
    {
        EffectiveInput += ReadEdgeScroll();
    }
    if (EffectiveInput.SizeSquared() > 1.0f)
    {
        EffectiveInput.Normalize();
    }

    const FRotationMatrix Heading(FRotator(0.0f, NewRotation.Yaw, 0.0f));
    const FVector Forward = Heading.GetUnitAxis(EAxis::X);
    const FVector Right = Heading.GetUnitAxis(EAxis::Y);
    const float ZoomRatio = FMath::Clamp(CameraArm->TargetArmLength / 6000.0f, 0.4f, 1.8f);
    const FVector DesiredVelocity = (Forward * EffectiveInput.X + Right * EffectiveInput.Y) * PanSpeed * ZoomRatio;
    PanVelocity = PanSmoothing > 0.0f
        ? FMath::VInterpTo(PanVelocity, DesiredVelocity, DeltaSeconds, PanSmoothing)
        : DesiredVelocity;

    const FVector DesiredLocation = GetActorLocation() + PanVelocity * DeltaSeconds;
    const FVector BoundedLocation = ClampToWorld(DesiredLocation);
    SetActorLocation(BoundedLocation);
    // Discard outward velocity at a boundary so changing direction responds immediately.
    if (!FMath::IsNearlyEqual(BoundedLocation.X, DesiredLocation.X))
    {
        PanVelocity.X = 0.0f;
    }
    if (!FMath::IsNearlyEqual(BoundedLocation.Y, DesiredLocation.Y))
    {
        PanVelocity.Y = 0.0f;
    }

    TargetZoom = FMath::Clamp(TargetZoom, MinimumZoom, FMath::Max(MinimumZoom, MaximumZoom));
    CameraArm->TargetArmLength = ZoomSmoothing > 0.0f
        ? FMath::FInterpTo(CameraArm->TargetArmLength, TargetZoom, DeltaSeconds, ZoomSmoothing)
        : TargetZoom;
}

void AWildlightCameraPawn::MoveCamera(float ForwardInput, float RightInput)
{
    MovementInput = bCameraInputEnabled
        ? FVector2D(FMath::Clamp(ForwardInput, -1.0f, 1.0f), FMath::Clamp(RightInput, -1.0f, 1.0f))
        : FVector2D::ZeroVector;
}

void AWildlightCameraPawn::RotateCamera(float Input)
{
    RotationInput = bCameraInputEnabled ? FMath::Clamp(Input, -1.0f, 1.0f) : 0.0f;
}

void AWildlightCameraPawn::ZoomCamera(float Input)
{
    if (bCameraInputEnabled && FMath::IsFinite(Input))
    {
        TargetZoom = FMath::Clamp(TargetZoom - Input * ZoomStep, MinimumZoom, FMath::Max(MinimumZoom, MaximumZoom));
    }
}

void AWildlightCameraPawn::FocusAt(FVector WorldLocation)
{
    if (WorldLocation.ContainsNaN())
    {
        return;
    }
    PanVelocity = FVector::ZeroVector;
    SetActorLocation(ClampToWorld(WorldLocation));
}

void AWildlightCameraPawn::SetWorldBounds(FVector2D Bounds)
{
    if (!FMath::IsFinite(Bounds.X) || !FMath::IsFinite(Bounds.Y))
    {
        return;
    }
    WorldBounds = FVector2D(FMath::Max(1.0, Bounds.X), FMath::Max(1.0, Bounds.Y));
    SetActorLocation(ClampToWorld(GetActorLocation()));
}

bool AWildlightCameraPawn::GetGroundTarget(FVector& OutLocation) const
{
    OutLocation = FVector::ZeroVector;
    const APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
    {
        return false;
    }

    FVector RayOrigin;
    FVector RayDirection;
    if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection)
        || FMath::IsNearlyZero(RayDirection.Z))
    {
        return false;
    }

    const double Distance = (static_cast<double>(GroundHeight) - RayOrigin.Z) / RayDirection.Z;
    if (Distance < 0.0 || !FMath::IsFinite(Distance))
    {
        return false;
    }

    const FVector Intersection = RayOrigin + RayDirection * Distance;
    if (Intersection.ContainsNaN()
        || Intersection.X < 0.0 || Intersection.X > WorldBounds.X
        || Intersection.Y < 0.0 || Intersection.Y > WorldBounds.Y)
    {
        return false;
    }

    OutLocation = Intersection;
    OutLocation.Z = GroundHeight;
    return true;
}

FVector2D AWildlightCameraPawn::ReadEdgeScroll() const
{
    const APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return FVector2D::ZeroVector;
    }

    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (ViewportWidth <= 0 || ViewportHeight <= 0
        || !PlayerController->GetMousePosition(MouseX, MouseY)
        || MouseX < 0.0f || MouseY < 0.0f
        || MouseX >= ViewportWidth || MouseY >= ViewportHeight)
    {
        return FVector2D::ZeroVector;
    }

    const float Border = FMath::Clamp(EdgeScrollPixels, 1.0f, FMath::Min(ViewportWidth, ViewportHeight) * 0.25f);
    FVector2D Input = FVector2D::ZeroVector;
    if (MouseY < Border)
    {
        Input.X = 1.0f - MouseY / Border;
    }
    else if (MouseY > ViewportHeight - Border)
    {
        Input.X = -(MouseY - (ViewportHeight - Border)) / Border;
    }
    if (MouseX < Border)
    {
        Input.Y = -(1.0f - MouseX / Border);
    }
    else if (MouseX > ViewportWidth - Border)
    {
        Input.Y = (MouseX - (ViewportWidth - Border)) / Border;
    }
    return Input;
}

FVector AWildlightCameraPawn::ClampToWorld(FVector Location) const
{
    Location.X = FMath::Clamp(Location.X, 0.0, static_cast<double>(WorldBounds.X));
    Location.Y = FMath::Clamp(Location.Y, 0.0, static_cast<double>(WorldBounds.Y));
    Location.Z = GroundHeight;
    return Location;
}
