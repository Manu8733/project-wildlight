#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "wildlight/Simulation.hpp"
#include "WildlightWorld.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class ADirectionalLight;
class ASkyLight;
class UAudioComponent;
class USoundBase;

UCLASS()
class WILDLIGHT_API AWildlightEntity : public AActor
{
    GENERATED_BODY()
public:
    AWildlightEntity();
    void Configure(UStaticMesh* Mesh, UMaterialInterface* MarkerMaterial, bool bUnit, FVector Scale, float FacingOffset = 0.f);
    void SetSelected(bool bSelected);
    void Animate(float Time, bool bMoving, bool bWorking, float Completion, bool bConnected);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Marker;
private:
    FVector BaseScale = FVector::OneVector;
    bool bIsUnit = false;
    float BaseYaw = 0.f;
};

UCLASS()
class WILDLIGHT_API AWildlightWorld : public AActor
{
    GENERATED_BODY()
public:
    AWildlightWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void StartMatch(int32 Map, wildlight::AIPersonality Personality, wildlight::Difficulty Difficulty);
    void RefreshPresentation();
    wildlight::CommandResult Submit(const wildlight::Command& Command);
    wildlight::CommandResult SetAutomaticGrowthPolicy(const wildlight::AutomaticGrowthPolicy& Policy);
    wildlight::Id Pick(wildlight::Vec2 Position, bool bIncludeResources = true) const;
    bool SaveMatch(const FString& Slot, FString& Error) const;
    bool LoadMatch(const FString& Slot, FString& Error);
    UStaticMesh* FindMesh(const FString& Identifier, UStaticMesh* Fallback) const;
    FVector ToWorld(wildlight::Vec2 Position, float Z = 0.f) const;
    FVector BuildingWorldPosition(const wildlight::Building& Building, float Z = 0.f) const;
    wildlight::Vec2 ToCell(FVector Position) const;
    void UpdatePreview(bool bVisible, wildlight::BuildingKind Kind, wildlight::Vec2 Position, uint8 Rotation);
    bool bRunning = false;
    bool bPaused = false;
    bool bTutorial = false;
    TSet<uint32> Selection;
    wildlight::Simulation Simulation;
    static constexpr float CellSize = 200.f;
private:
    void BuildArena();
    void UpdateLightLinks();
    UHierarchicalInstancedStaticMeshComponent* MakeInstances(const FName& Name, UStaticMesh* Mesh, UMaterialInterface* Material);
    UMaterialInstanceDynamic* ColorMaterial(FLinearColor Color, float Emission = 0.f);
    UStaticMeshComponent* MakeMesh(const FName& Name, UStaticMesh* Mesh, UMaterialInterface* Material);
    void PlayCue(int32 Index, FVector Position, bool bSpatial);
    void PresentEvents();
    void InitializeEventCursorFromSave();
    UStaticMesh* FindBuildingMesh(const wildlight::Building& Building);
    void ClearDecorationsForBuilding(const wildlight::Building& Building);
    UPROPERTY() TMap<uint32, TObjectPtr<AWildlightEntity>> Visuals;
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> TerrainGroups;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> LightLinks;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Fireflies;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ForestMesh;
    UPROPERTY() TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TuftMesh;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Preview;
    UPROPERTY() TObjectPtr<ADirectionalLight> Sun;
    UPROPERTY() TObjectPtr<ASkyLight> Ambient;
    UPROPERTY() TObjectPtr<UStaticMesh> Cube;
    UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
    UPROPERTY() TObjectPtr<UStaticMesh> Cylinder;
    UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> PlayerMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> EnemyMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> LightMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> ValidMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> InvalidMaterial;
    UPROPERTY() TObjectPtr<UAudioComponent> Ambience;
    UPROPERTY() TArray<TObjectPtr<USoundBase>> Sounds;
    float RefreshTimer = 0.f;
    float AutosaveTimer = 0.f;
    float VisualTime = 0.f;
    TArray<FVector> FireflyOrigins;
    TMap<uint32, double> PreviousHealth;
    TMap<uint32, uint8> PresentedBuildingLevels;
    TSet<FString> MissingBuildingTierWarnings;
    float LastImpactTime = -10.f;
    uint64 LastEventTick = 0;
    TSet<FString> EventsAtLastTick;
    wildlight::MatchOutcome PreviousOutcome = wildlight::MatchOutcome::Playing;
};
