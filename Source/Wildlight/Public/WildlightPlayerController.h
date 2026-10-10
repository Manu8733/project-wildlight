#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "wildlight/Simulation.hpp"
#include "WildlightPlayerController.generated.h"

class AWildlightWorld;
UCLASS()
class WILDLIGHT_API AWildlightPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    void HandleUIAction(const FString& Action);
    void StartNewGame(bool bTutorial);
    void Notify(const FString& Text);
    FString TutorialText() const;
    bool IsDragging() const { return bDragging; }
    FVector2D DragOrigin() const { return DragStart; }
    FVector2D MousePosition() const;
    bool GroundPosition(wildlight::Vec2& Out) const;
    bool bMainMenu = true;
    bool bSettings = false;
    bool bKeyBindings = false;
    bool bCapturingBinding = false;
    FName CaptureRole = NAME_None;
    FString CaptureLabel;
    int32 MapChoice = 0;
    int32 PersonalityChoice = 0;
    int32 DifficultyChoice = 1;
    FString Status;
    bool bBuildPreview = false;
    wildlight::BuildingKind BuildKind = wildlight::BuildingKind::House;
    bool bSpellPreview = false;
    bool bPatrolPreview = false;
    wildlight::SpellKind SpellKind = wildlight::SpellKind::RootBarrier;
    uint8 BuildRotation = 0;
    UPROPERTY() TObjectPtr<AWildlightWorld> Arena;
private:
    void SelectPressed();
    void SelectReleased();
    void ContextPressed();
    void PausePressed();
    void BuildPressed();
    void RotatePressed();
    void SavePressed();
    void LoadPressed();
    void ZoomIn();
    void ZoomOut();
    void Submit(wildlight::Command Command);
    void RecallGroup(int32 Index);
    void BeginBindingCapture(FName Role);
    void CancelBindingCapture();
    void TickBindingCapture(float DeltaSeconds);
    float CaptureGraceRemaining = 0.f;
    bool bDragging = false;
    bool bUIConsumed = false;
    FVector2D DragStart;
    float StatusSeconds = 0.f;
    float LastGroupTime = -100.f;
    int32 LastGroup = -1;
    TArray<uint32> Groups[9];
};
