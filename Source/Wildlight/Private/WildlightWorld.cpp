#include "WildlightWorld.h"
#include "WildlightGameInstance.h"
#include "WildlightCameraPawn.h"
#include "WildlightPlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/SkyAtmosphere.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
const TCHAR* BuildingNames[] = {TEXT("Castle"), TEXT("House"), TEXT("Farm"), TEXT("Mill"), TEXT("Depot"), TEXT("LumberCamp"), TEXT("Quarry"), TEXT("GoldMine"), TEXT("Barracks"), TEXT("ArcherRange"), TEXT("Forge"), TEXT("Market"), TEXT("Wall"), TEXT("Gate"), TEXT("Watchtower"), TEXT("Beacon"), TEXT("Sanctuary"), TEXT("MageTower"), TEXT("SiegeWorkshop")};
const TCHAR* UnitNames[] = {TEXT("Worker"), TEXT("Militia"), TEXT("Spearman"), TEXT("Swordsman"), TEXT("Archer"), TEXT("Cavalry"), TEXT("Engineer"), TEXT("ArcaneGuard"), TEXT("ForestAlly"), TEXT("Ram"), TEXT("Catapult"), TEXT("Wizard")};
FString SavePath(const FString& Slot)
{
    // Caller controls slots; no paths or user-entered filenames enter the filesystem API.
    const FString Name = Slot == TEXT("Autosave") ? TEXT("Autosave") : TEXT("Quicksave");
    return FPaths::ProjectSavedDir() / TEXT("SaveGames") / (Name + TEXT(".wildlight"));
}
wildlight::Vec2 FootprintCenter(wildlight::BuildingKind Kind, wildlight::Vec2 P, uint8 Rotation)
{
    const auto& D = wildlight::definition(Kind);
    const int32 W = Rotation % 2 ? D.height : D.width;
    const int32 H = Rotation % 2 ? D.width : D.height;
    return {FMath::FloorToDouble(P.x) - W / 2 + W * .5, FMath::FloorToDouble(P.y) - H / 2 + H * .5};
}
bool WithinFootprint(const wildlight::Building& B, wildlight::Vec2 P)
{
    const auto& D = wildlight::definition(B.kind);
    const int32 W = B.rotation % 2 ? D.height : D.width;
    const int32 H = B.rotation % 2 ? D.width : D.height;
    const int32 Left = FMath::FloorToInt(B.position.x) - W / 2;
    const int32 Top = FMath::FloorToInt(B.position.y) - H / 2;
    return P.x >= Left && P.x < Left + W && P.y >= Top && P.y < Top + H;
}
FString EventPresentationKey(const wildlight::Event& Event)
{
    return FString::Printf(TEXT("%u/%u/%s"), static_cast<uint32>(Event.owner), Event.subject_id, UTF8_TO_TCHAR(Event.text.c_str()));
}
}

AWildlightEntity::AWildlightEntity()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("EntityRoot")));
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(GetRootComponent());
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
    Marker->SetupAttachment(Body);
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetCastShadow(false);
}
void AWildlightEntity::Configure(UStaticMesh* Mesh, UMaterialInterface* MarkerMaterial, bool bUnit, FVector Scale, float FacingOffset)
{
    bIsUnit = bUnit;
    Body->SetStaticMesh(Mesh);
    BaseScale = Scale;
    BaseYaw = FacingOffset;
    Body->SetRelativeScale3D(Scale);
    Body->SetRelativeRotation(FRotator(0, BaseYaw, 0));
    Marker->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    Marker->SetMaterial(0, MarkerMaterial);
    // Absolute scaling keeps the badge readable on imported models of different dimensions.
    Marker->SetAbsolute(false, false, true);
    Marker->SetRelativeScale3D(bUnit ? FVector(0.8f, 0.8f, 0.025f) : FVector(2.f, 2.f, 0.025f));
    Marker->SetRelativeLocation(FVector(0, 0, 7));
}
void AWildlightEntity::SetSelected(bool bSelected)
{
    Marker->SetVisibility(true);
    const float Radius = bIsUnit ? (bSelected ? 1.f : .45f) : (bSelected ? 2.5f : 1.2f);
    Marker->SetWorldScale3D(FVector(Radius, Radius, .025f));
}
void AWildlightEntity::Animate(float Time, bool bMoving, bool bWorking, float Completion, bool bConnected)
{
    if (bIsUnit)
    {
        const float Bob = bMoving ? FMath::Sin(Time * 11.f) * 4.f : FMath::Sin(Time * 2.f) * 0.7f;
        Body->SetRelativeScale3D(BaseScale);
        SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, 8.f + Bob));
        Body->SetRelativeRotation(FRotator(0, BaseYaw, bWorking ? FMath::Sin(Time * 9.f) * 8.f : 0.f));
    }
    else
    {
        // Readable construction reveal; not a substitute for a finished animation set.
        Body->SetRelativeScale3D(BaseScale * FVector(1, 1, FMath::Max(0.12f, Completion)));
    }
    (void)bConnected;
}

AWildlightWorld::AWildlightWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("WorldRoot")));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Cube = CubeAsset.Object;
    Sphere = SphereAsset.Object;
    Cylinder = CylinderAsset.Object;
}
void AWildlightWorld::BeginPlay()
{
    Super::BeginPlay();
    BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Wildlight/Materials/M_Wildlight.M_Wildlight"));
    if (!BaseMaterial) BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    PlayerMaterial = ColorMaterial(FLinearColor(0.32f, 0.68f, 0.73f), 0.3f);
    EnemyMaterial = ColorMaterial(FLinearColor(0.79f, 0.36f, 0.26f), 0.3f);
    LightMaterial = ColorMaterial(FLinearColor(0.95f, 0.70f, 0.30f), 1.3f);
    ValidMaterial = ColorMaterial(FLinearColor(0.25f, 0.9f, 0.4f), 0.3f);
    InvalidMaterial = ColorMaterial(FLinearColor(0.95f, 0.16f, 0.17f), 0.3f);
    Preview = MakeMesh(TEXT("BuildPreview"), Cube, ValidMaterial);
    Preview->SetVisibility(false);
    Preview->SetCastShadow(false);
    Sun = GetWorld()->SpawnActor<ADirectionalLight>();
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sun->GetLightComponent()->SetIntensity(3.f);
    Sun->SetActorRotation(FRotator(-48.f, -35.f, 0));
    if (auto* Directional = Cast<UDirectionalLightComponent>(Sun->GetLightComponent())) Directional->bAtmosphereSunLight = true;
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    if (auto* Fog = GetWorld()->SpawnActor<AExponentialHeightFog>())
    {
        Fog->GetComponent()->SetFogDensity(.004f);
        Fog->GetComponent()->SetFogHeightFalloff(.35f);
    }
    Ambient = GetWorld()->SpawnActor<ASkyLight>();
    Ambient->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Ambient->GetLightComponent()->SetIntensity(0.55f);
    Ambient->GetLightComponent()->SetLightColor(FLinearColor(0.64f, 0.78f, 1.f));
    Ambient->GetLightComponent()->RecaptureSky();
    const TCHAR* AudioNames[] = {TEXT("A_Command"), TEXT("A_Build"), TEXT("A_Magic"), TEXT("A_Alert"), TEXT("A_Impact"), TEXT("A_Victory"), TEXT("A_Defeat"), TEXT("A_WoodlandLoop")};
    for (const TCHAR* AudioName : AudioNames)
    {
        const FString Name(AudioName);
        Sounds.Add(LoadObject<USoundBase>(nullptr, *(TEXT("/Game/Wildlight/Audio/") + Name + TEXT(".") + Name)));
    }
    if (Sounds[7]) Ambience = UGameplayStatics::SpawnSound2D(this, Sounds[7], .2f, 1.f, 0.f, nullptr, false, false);
    StartMatch(0, wildlight::AIPersonality::Builder, wildlight::Difficulty::Normal);
    bRunning = false;
    if (auto* Camera = Cast<AWildlightCameraPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
        Camera->FocusAt(ToWorld(wildlight::map_catalog()[0].player_start));
}
UMaterialInstanceDynamic* AWildlightWorld::ColorMaterial(FLinearColor Color, float Emission)
{
    UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    if (!Material) return nullptr;
    Material->SetVectorParameterValue(TEXT("Tint"), Color);
    Material->SetVectorParameterValue(TEXT("Color"), Color);
    Material->SetScalarParameterValue(TEXT("Emission"), Emission);
    Materials.Add(Material);
    return Material;
}
UStaticMesh* AWildlightWorld::FindMesh(const FString& Identifier, UStaticMesh* Fallback) const
{
    const FString Name = TEXT("SM_") + Identifier;
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *(TEXT("/Game/Wildlight/Meshes/") + Name + TEXT(".") + Name));
    return Mesh ? Mesh : Fallback;
}
UStaticMesh* AWildlightWorld::FindBuildingMesh(const wildlight::Building& Building)
{
    const bool bHouse = Building.kind == wildlight::BuildingKind::House;
    const bool bFarm = Building.kind == wildlight::BuildingKind::Farm;
    if (!bHouse && !bFarm) return FindMesh(BuildingNames[static_cast<int32>(Building.kind)], Cube);
    const TCHAR* HouseTiers[] = {TEXT("SM_House_L1"), TEXT("SM_House_L2"), TEXT("SM_House_L3")};
    const TCHAR* FarmTiers[] = {TEXT("SM_Farm_L1"), TEXT("SM_Farm_L2"), TEXT("SM_Farm_L3")};
    const int32 Tier = FMath::Clamp(static_cast<int32>(Building.level) - 1, 0, 2);
    const FString Name(bHouse ? HouseTiers[Tier] : FarmTiers[Tier]);
    const FString Path = TEXT("/Game/Wildlight/Meshes/") + Name + TEXT(".") + Name;
    if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet)) return Mesh;
    if (!MissingBuildingTierWarnings.Contains(Name))
    {
        MissingBuildingTierWarnings.Add(Name);
        UE_LOG(LogTemp, Warning, TEXT("Missing building tier %s; using prototype fallback. Final level art is unavailable."), *Name);
    }
    const FString PrototypeName = bHouse ? TEXT("SM_House") : TEXT("SM_Farm");
    const FString PrototypePath = TEXT("/Game/Wildlight/Meshes/") + PrototypeName + TEXT(".") + PrototypeName;
    UStaticMesh* Prototype = LoadObject<UStaticMesh>(nullptr, *PrototypePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    return Prototype ? Prototype : Cube;
}
UStaticMeshComponent* AWildlightWorld::MakeMesh(const FName& Name, UStaticMesh* Mesh, UMaterialInterface* Material)
{
    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), Name));
    AddInstanceComponent(Component);
    Component->SetupAttachment(GetRootComponent());
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (Material) Component->SetMaterial(0, Material);
    Component->RegisterComponent();
    return Component;
}
UHierarchicalInstancedStaticMeshComponent* AWildlightWorld::MakeInstances(const FName& Name, UStaticMesh* Mesh, UMaterialInterface* Material)
{
    UHierarchicalInstancedStaticMeshComponent* Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), Name));
    AddInstanceComponent(Component);
    Component->SetupAttachment(GetRootComponent());
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (Material) Component->SetMaterial(0, Material);
    Component->SetCullDistances(0, 22000);
    Component->RegisterComponent();
    TerrainGroups.Add(Component);
    return Component;
}
FVector AWildlightWorld::ToWorld(wildlight::Vec2 P, float Z) const
{
    return FVector(P.x * CellSize, P.y * CellSize, Z);
}
wildlight::Vec2 AWildlightWorld::ToCell(FVector P) const { return {P.X / CellSize, P.Y / CellSize}; }
FVector AWildlightWorld::BuildingWorldPosition(const wildlight::Building& B, float Z) const
{
    return ToWorld(FootprintCenter(B.kind, B.position, B.rotation), Z);
}
void AWildlightWorld::StartMatch(int32 Map, wildlight::AIPersonality Personality, wildlight::Difficulty Difficulty)
{
    for (auto& Pair : Visuals) if (IsValid(Pair.Value)) Pair.Value->Destroy();
    Visuals.Empty();
    Selection.Empty();
    Simulation.new_match(FMath::Clamp(Map, 0, 2), 0x57494c44u, Personality, Difficulty);
    bRunning = true;
    bPaused = false;
    AutosaveTimer = 0.f;
    PreviousHealth.Empty();
    PresentedBuildingLevels.Empty();
    LastEventTick = 0; EventsAtLastTick.Empty();
    PreviousOutcome = wildlight::MatchOutcome::Playing;
    BuildArena();
    RefreshPresentation();
}
void AWildlightWorld::BuildArena()
{
    for (auto& Component : TerrainGroups) if (Component) Component->DestroyComponent();
    TerrainGroups.Empty();
    Materials.Empty();
    Fireflies = nullptr;
    FireflyOrigins.Empty();
    const auto& State = Simulation.state();
    auto* Grass = MakeInstances(TEXT("GrassTiles"), Cube, ColorMaterial(FLinearColor(0.27f, 0.43f, 0.28f)));
    auto* Meadow = MakeInstances(TEXT("MeadowPatches"), Sphere, ColorMaterial(FLinearColor(0.31f, 0.47f, 0.30f)));
    auto* Tufts = MakeInstances(TEXT("GrassTufts"), FindMesh(TEXT("GrassTuft"), Sphere), nullptr);
    TuftMesh = Tufts;
    auto* Water = MakeInstances(TEXT("RiverTiles"), Cube, ColorMaterial(FLinearColor(0.10f, 0.43f, 0.53f), 0.12f));
    auto* Rock = MakeInstances(TEXT("CliffTiles"), FindMesh(TEXT("Rock"), Cube), nullptr);
    auto* Trees = MakeInstances(TEXT("Forest"), FindMesh(TEXT("TreePine"), Cylinder), nullptr);
    ForestMesh = Trees;
    auto* Bridge = MakeInstances(TEXT("Bridges"), FindMesh(TEXT("Bridge"), Cube), nullptr);
    auto* Deck = MakeInstances(TEXT("BridgeDeck"), Cube, ColorMaterial(FLinearColor(.36f, .25f, .14f)));
    auto* Ruins = MakeInstances(TEXT("Ruins"), FindMesh(TEXT("Ruin"), Cylinder), nullptr);
    Fireflies = MakeInstances(TEXT("Fireflies"), Sphere, LightMaterial);
    Trees->SetCullDistances(0, 15000);
    FRandomStream Random(771 + State.map_index);
    for (int32 Y = 0; Y < State.height; ++Y)
    for (int32 X = 0; X < State.width; ++X)
    {
        const auto Terrain = State.terrain[Y * State.width + X];
        const FVector P((X + .5f) * CellSize, (Y + .5f) * CellSize, -16.f);
        auto* Ground = Terrain == wildlight::Terrain::Water || Terrain == wildlight::Terrain::Bridge ? Water : Grass;
        Ground->AddInstance(FTransform(FRotator::ZeroRotator, P, FVector(2.f, 2.f, .3f)));
        bool bCovered = false;
        for (const auto& B : State.buildings) if (B.health > 0 && WithinFootprint(B, {(X + .5), (Y + .5)})) { bCovered = true; break; }
        if (Terrain == wildlight::Terrain::Grass && (X * 19 + Y * 37) % 67 == 0)
            Meadow->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 17), FVector(Random.FRandRange(4.f, 7.f), Random.FRandRange(3.f, 5.f), .02f)));
        if (!bCovered && (Terrain == wildlight::Terrain::Grass || Terrain == wildlight::Terrain::Forest) && (X * 3 + Y * 7) % 11 == 0)
            Tufts->AddInstance(FTransform(FRotator(0, Random.FRandRange(0, 360), 0), P + FVector(Random.FRandRange(-70, 70), Random.FRandRange(-70, 70), 16), FVector(.8f)));
        if (!bCovered && Terrain == wildlight::Terrain::Forest && (X + Y) % 2 == 0)
        {
            const float S = Random.FRandRange(.8f, 1.3f);
            Trees->AddInstance(FTransform(FRotator(0, Random.FRandRange(0, 360), 0), P + FVector(Random.FRandRange(-40, 40), Random.FRandRange(-40, 40), 16), FVector(S)));
            if ((X * 5 + Y) % 9 == 0)
            {
                const FVector Origin = P + FVector(40, 10, Random.FRandRange(70, 180));
                FireflyOrigins.Add(Origin);
                Fireflies->AddInstance(FTransform(FRotator::ZeroRotator, Origin, FVector(.035f)));
            }
        }
        if (Terrain == wildlight::Terrain::Rock) Rock->AddInstance(FTransform(FRotator(0, Random.FRandRange(0, 360), 0), P + FVector(0, 0, 16), FVector(1.4f)));
        if (Terrain == wildlight::Terrain::Bridge) Deck->AddInstance(FTransform(FRotator::ZeroRotator, P + FVector(0, 0, 20), FVector(2.f, 2.f, .08f)));
        if (Terrain == wildlight::Terrain::Ruin && (X + Y) % 3 == 0) Ruins->AddInstance(FTransform(FRotator(0, 0, 0), P + FVector(0, 0, 16), FVector(.7f)));
    }
    // One authored bridge per connected crossing, not one overlapping prefab per cell.
    TSet<int32> Visited;
    for (int32 Start = 0; Start < State.width * State.height; ++Start)
    {
        if (State.terrain[Start] != wildlight::Terrain::Bridge || Visited.Contains(Start)) continue;
        TArray<int32> Queue; Queue.Add(Start); Visited.Add(Start);
        int32 MinX = Start % State.width, MaxX = MinX, MinY = Start / State.width, MaxY = MinY;
        for (int32 Q = 0; Q < Queue.Num(); ++Q)
        {
            const int32 X = Queue[Q] % State.width, Y = Queue[Q] / State.width;
            MinX = FMath::Min(MinX, X); MaxX = FMath::Max(MaxX, X); MinY = FMath::Min(MinY, Y); MaxY = FMath::Max(MaxY, Y);
            const FIntPoint Neighbours[] = {{X - 1, Y}, {X + 1, Y}, {X, Y - 1}, {X, Y + 1}};
            for (const auto& N : Neighbours)
            {
                if (N.X < 0 || N.Y < 0 || N.X >= State.width || N.Y >= State.height) continue;
                const int32 Index = N.Y * State.width + N.X;
                if (!Visited.Contains(Index) && State.terrain[Index] == wildlight::Terrain::Bridge) { Visited.Add(Index); Queue.Add(Index); }
            }
        }
        const FVector Size = Bridge->GetStaticMesh()->GetBounds().BoxExtent * 2;
        const float SpanX = (MaxX - MinX + 1) * CellSize, SpanY = (MaxY - MinY + 1) * CellSize;
        Bridge->AddInstance(FTransform(FRotator(0, 90, 0), FVector((MinX + MaxX + 1) * CellSize * .5f, (MinY + MaxY + 1) * CellSize * .5f, 8), FVector(SpanY / FMath::Max(Size.X, 1.), SpanX / FMath::Max(Size.Y, 1.), 1.)));
    }
    Fireflies->SetCastShadow(false);
}
void AWildlightWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    VisualTime += DeltaSeconds;
    if (bRunning && !bPaused && Simulation.state().outcome == wildlight::MatchOutcome::Playing)
    {
        Simulation.step(FMath::Min(DeltaSeconds, 0.25f));
        AutosaveTimer += DeltaSeconds;
        if (AutosaveTimer >= 180.f)
        {
            AutosaveTimer = 0.f;
            FString Error;
            if (!SaveMatch(TEXT("Autosave"), Error)) UE_LOG(LogTemp, Warning, TEXT("Wildlight autosave: %s"), *Error);
        }
    }
    RefreshTimer += DeltaSeconds;
    if (RefreshTimer >= 0.08f)
    {
        RefreshTimer = 0.f;
        RefreshPresentation();
        if (Fireflies && Fireflies->IsVisible())
            for (int32 I = 0; I < FireflyOrigins.Num(); ++I)
            {
                const float Phase = VisualTime * .8f + I * 1.618f;
                const FVector P = FireflyOrigins[I] + FVector(FMath::Sin(Phase) * 38.f, FMath::Cos(Phase * .7f) * 26.f, FMath::Sin(Phase * 1.3f) * 20.f);
                Fireflies->UpdateInstanceTransform(I, FTransform(FRotator::ZeroRotator, P, FVector(.027f + .008f * FMath::Sin(Phase))), false, I == FireflyOrigins.Num() - 1, true);
            }
    }
    const float Day = static_cast<float>(Simulation.day_fraction());
    // A bright ambient floor preserves RTS readability throughout the cycle.
    const float Sunlight = FMath::Max(.35f, FMath::Sin(Day * 2.f * PI));
    if (Sun) Sun->GetLightComponent()->SetIntensity(.6f + Sunlight * 2.8f);
    if (Sun) Sun->GetLightComponent()->SetLightColor(FLinearColor::LerpUsingHSV(FLinearColor(.42f, .61f, 1.f), FLinearColor(1.f, .88f, .64f), Sunlight));
    const auto* Preferences = Cast<UWildlightGameInstance>(GetGameInstance());
    if (Fireflies) Fireflies->SetVisibility(Sunlight < .75f && (!Preferences || !Preferences->bReducedEffects));
    if (Ambience) Ambience->SetVolumeMultiplier((Preferences ? Preferences->MasterVolume : .65f) * (bPaused ? .10f : .27f));
    if (Simulation.state().outcome != PreviousOutcome)
    {
        PreviousOutcome = Simulation.state().outcome;
        if (PreviousOutcome != wildlight::MatchOutcome::Playing) PlayCue(PreviousOutcome == wildlight::MatchOutcome::PlayerVictory ? 5 : 6, FVector::ZeroVector, false);
    }
}
void AWildlightWorld::RefreshPresentation()
{
    const auto& State = Simulation.state();
    TSet<uint32> Alive;
    for (const auto& B : State.buildings)
    {
        Alive.Add(B.id);
        AWildlightEntity* V = Visuals.FindRef(B.id);
        if (!V)
        {
            V = GetWorld()->SpawnActor<AWildlightEntity>();
            Visuals.Add(B.id, V);
            ClearDecorationsForBuilding(B);
        }
        const uint8* PresentedLevel = PresentedBuildingLevels.Find(B.id);
        if (!PresentedLevel || *PresentedLevel != B.level)
        {
            const auto& D = wildlight::definition(B.kind);
            UStaticMesh* Mesh = FindBuildingMesh(B);
            const bool bFallback = Mesh == Cube;
            const FVector Bounds = Mesh->GetBounds().BoxExtent * 2;
            const bool bGate = B.kind == wildlight::BuildingKind::Gate && !bFallback;
            const double MeshWidth = bGate ? Bounds.Y : Bounds.X, MeshHeight = bGate ? Bounds.X : Bounds.Y;
            const double Fit = FMath::Min((D.width * CellSize - 4.) / FMath::Max(MeshWidth, 1.), (D.height * CellSize - 4.) / FMath::Max(MeshHeight, 1.));
            V->Body->EmptyOverrideMaterials();
            V->Configure(Mesh, B.owner == wildlight::Owner::Player ? PlayerMaterial : EnemyMaterial, false, bFallback ? FVector(D.width * 2.f, D.height * 2.f, 2.5f) : FVector(FMath::Min(Fit, 1.3)), bGate ? 90.f : 0.f);
            if (bFallback) V->Body->SetMaterial(0, B.owner == wildlight::Owner::Player ? PlayerMaterial : EnemyMaterial);
            PresentedBuildingLevels.Add(B.id, B.level);
        }
        V->SetActorLocation(BuildingWorldPosition(B));
        V->SetActorRotation(FRotator(0, B.rotation * 90.f, 0));
        V->SetActorHiddenInGame(B.owner == wildlight::Owner::Enemy && !Simulation.is_visible(wildlight::Owner::Player, B.position));
        V->SetSelected(Selection.Contains(B.id));
        V->Animate(VisualTime, false, false, B.progress, B.connected);
        if (const double* Health = PreviousHealth.Find(B.id); Health && B.health < *Health - .01 && VisualTime - LastImpactTime > .2f && Simulation.is_visible(wildlight::Owner::Player, B.position))
        {
            PlayCue(4, BuildingWorldPosition(B), true); LastImpactTime = VisualTime;
        }
        PreviousHealth.Add(B.id, B.health);
    }
    for (const auto& U : State.units)
    {
        Alive.Add(U.id);
        AWildlightEntity* V = Visuals.FindRef(U.id);
        if (!V)
        {
            V = GetWorld()->SpawnActor<AWildlightEntity>();
            UStaticMesh* Mesh = FindMesh(UnitNames[static_cast<int32>(U.kind)], Cylinder);
            V->Configure(Mesh, U.owner == wildlight::Owner::Player ? PlayerMaterial : EnemyMaterial, true, Mesh == Cylinder ? FVector(.5f, .5f, 1.f) : FVector::OneVector, 90.f);
            if (Mesh == Cylinder) V->Body->SetMaterial(0, U.owner == wildlight::Owner::Player ? PlayerMaterial : EnemyMaterial);
            Visuals.Add(U.id, V);
        }
        const FVector NewLocation = ToWorld(U.position, 8.f);
        const FVector Delta = NewLocation - V->GetActorLocation();
        if (Delta.SizeSquared2D() > 1.f) V->SetActorRotation(FRotator(0, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0));
        V->SetActorLocation(NewLocation);
        V->SetActorHiddenInGame(!Simulation.is_unit_visible(wildlight::Owner::Player, U.id));
        V->SetSelected(Selection.Contains(U.id));
        V->Animate(VisualTime + U.id, Delta.SizeSquared2D() > 1.f, U.order == wildlight::OrderType::Gather || U.order == wildlight::OrderType::Attack, 1.f, false);
        if (const double* Health = PreviousHealth.Find(U.id); Health && U.health < *Health - .01 && VisualTime - LastImpactTime > .2f && Simulation.is_visible(wildlight::Owner::Player, U.position))
        {
            PlayCue(4, ToWorld(U.position), true); LastImpactTime = VisualTime;
        }
        PreviousHealth.Add(U.id, U.health);
    }
    for (const auto& N : State.nodes)
    {
        if (N.remaining <= 0) continue;
        Alive.Add(N.id);
        AWildlightEntity* V = Visuals.FindRef(N.id);
        if (!V)
        {
            V = GetWorld()->SpawnActor<AWildlightEntity>();
            UStaticMesh* Mesh = FindMesh(N.kind == wildlight::ResourceKind::Wood ? TEXT("TreeOak") : N.kind == wildlight::ResourceKind::Stone ? TEXT("Rock") : TEXT("Crystal"), Sphere);
            V->Configure(Mesh, LightMaterial, false, Mesh == Sphere ? FVector(.8f) : FVector(.8f));
            Visuals.Add(N.id, V);
        }
        V->SetActorLocation(ToWorld(N.position));
        const int32 X = FMath::Clamp(static_cast<int32>(N.position.x), 0, State.width - 1);
        const int32 Y = FMath::Clamp(static_cast<int32>(N.position.y), 0, State.height - 1);
        V->SetActorHiddenInGame(!State.explored[0][Y * State.width + X]);
        V->Marker->SetVisibility(false);
    }
    for (const auto& E : State.effects)
    {
        Alive.Add(E.id);
        AWildlightEntity* V = Visuals.FindRef(E.id);
        if (!V)
        {
            V = GetWorld()->SpawnActor<AWildlightEntity>();
            V->Configure(E.kind == wildlight::SpellKind::LightBridge ? Cube : Cylinder, LightMaterial, false, E.kind == wildlight::SpellKind::LightBridge ? FVector(2.f, 8.f, .15f) : FVector(E.radius * 4.f, E.radius * 4.f, E.kind == wildlight::SpellKind::RootBarrier ? 2.f : .1f));
            V->Body->SetMaterial(0, E.owner == wildlight::Owner::Player ? LightMaterial : EnemyMaterial);
            Visuals.Add(E.id, V);
        }
        V->SetActorLocation(ToWorld(E.position, 12.f));
        V->SetActorHiddenInGame(!Simulation.is_visible(wildlight::Owner::Player, E.position));
        V->Marker->SetVisibility(false);
    }
    for (const auto& P : State.projectiles)
    {
        Alive.Add(P.id);
        AWildlightEntity* V = Visuals.FindRef(P.id);
        if (!V)
        {
            V = GetWorld()->SpawnActor<AWildlightEntity>();
            V->Configure(Sphere, LightMaterial, false, FVector(P.siege ? .22f : .07f));
            V->Body->SetMaterial(0, LightMaterial);
            Visuals.Add(P.id, V);
        }
        V->SetActorLocation(ToWorld(P.position, P.siege ? 180.f : 65.f));
        V->SetActorHiddenInGame(!Simulation.is_visible(wildlight::Owner::Player, P.position));
        V->Marker->SetVisibility(false);
    }
    TArray<uint32> Remove;
    for (const auto& Pair : Visuals) if (!Alive.Contains(Pair.Key)) Remove.Add(Pair.Key);
    for (uint32 Id : Remove) { if (Visuals[Id]) Visuals[Id]->Destroy(); Visuals.Remove(Id); Selection.Remove(Id); PreviousHealth.Remove(Id); PresentedBuildingLevels.Remove(Id); }
    UpdateLightLinks();
    PresentEvents();
}
void AWildlightWorld::UpdateLightLinks()
{
    int32 Index = 0;
    const auto& Buildings = Simulation.state().buildings;
    for (size_t I = 0; I < Buildings.size(); ++I)
    for (size_t J = I + 1; J < Buildings.size(); ++J)
    {
        const auto& A = Buildings[I]; const auto& B = Buildings[J];
        double RadiusA = wildlight::definition(A.kind).light_radius;
        double RadiusB = wildlight::definition(B.kind).light_radius;
        for (const auto& Effect : Simulation.state().effects)
            if (Effect.kind == wildlight::SpellKind::Amplify && Effect.owner == A.owner && Effect.remaining > 0)
            {
                if (FVector::Dist2D(ToWorld(Effect.position), ToWorld(A.position)) <= Effect.radius * CellSize) RadiusA += 5;
                if (FVector::Dist2D(ToWorld(Effect.position), ToWorld(B.position)) <= Effect.radius * CellSize) RadiusB += 5;
            }
        const float Radius = static_cast<float>(FMath::Max(RadiusA, RadiusB));
        if (A.owner != B.owner || !A.connected || !B.connected || Radius <= 0 || A.progress < 1 || B.progress < 1) continue;
        if (wildlight::definition(A.kind).light_radius <= 0 || wildlight::definition(B.kind).light_radius <= 0) continue;
        if (FVector::Dist2D(ToWorld(A.position), ToWorld(B.position)) > Radius * CellSize) continue;
        const FVector From = BuildingWorldPosition(A, 120.f), To = BuildingWorldPosition(B, 120.f);
        if (A.owner == wildlight::Owner::Enemy && (!Simulation.is_visible(wildlight::Owner::Player, A.position) || !Simulation.is_visible(wildlight::Owner::Player, B.position))) continue;
        if (Index == LightLinks.Num()) LightLinks.Add(MakeMesh(*FString::Printf(TEXT("LightLink_%d"), Index), Cylinder, LightMaterial));
        UStaticMeshComponent* Beam = LightLinks[Index++];
        const FVector Direction = To - From;
        Beam->SetWorldLocation((From + To) * .5f);
        Beam->SetWorldRotation(FRotationMatrix::MakeFromZ(Direction).Rotator());
        Beam->SetWorldScale3D(FVector(.055f, .055f, Direction.Size() / 100.f));
        Beam->SetMaterial(0, A.owner == wildlight::Owner::Player ? LightMaterial : EnemyMaterial);
        Beam->SetCastShadow(false);
        Beam->SetVisibility(true);
    }
    for (; Index < LightLinks.Num(); ++Index) LightLinks[Index]->SetVisibility(false);
}
wildlight::Id AWildlightWorld::Pick(wildlight::Vec2 Position, bool bResources) const
{
    wildlight::Id Best = 0;
    double BestDistance = 1.0;
    for (const auto& U : Simulation.state().units)
    {
        if (!Simulation.is_unit_visible(wildlight::Owner::Player, U.id)) continue;
        const double D = FMath::Square(U.position.x - Position.x) + FMath::Square(U.position.y - Position.y);
        if (D < BestDistance) { BestDistance = D; Best = U.id; }
    }
    if (Best) return Best;
    for (const auto& B : Simulation.state().buildings)
    {
        if (B.owner == wildlight::Owner::Enemy && !Simulation.is_visible(wildlight::Owner::Player, B.position)) continue;
        const auto& D = wildlight::definition(B.kind);
        const int32 Width = (B.rotation % 2) ? D.height : D.width;
        const int32 Height = (B.rotation % 2) ? D.width : D.height;
        const double Left = FMath::FloorToDouble(B.position.x) - Width / 2;
        const double Top = FMath::FloorToDouble(B.position.y) - Height / 2;
        if (Position.x >= Left && Position.x < Left + Width && Position.y >= Top && Position.y < Top + Height) return B.id;
    }
    if (bResources) for (const auto& N : Simulation.state().nodes)
    {
        if (N.remaining <= 0 || !Simulation.is_visible(wildlight::Owner::Player, N.position)) continue;
        const double D = FMath::Square(N.position.x - Position.x) + FMath::Square(N.position.y - Position.y);
        if (D < BestDistance) { BestDistance = D; Best = N.id; }
    }
    return Best;
}
wildlight::CommandResult AWildlightWorld::Submit(const wildlight::Command& Command)
{
    auto Result = Simulation.issue(Command);
    if (Result.accepted)
    {
        RefreshPresentation();
        // Upgrades use their event cue, including automatic transitions.
        if (Command.type != wildlight::CommandType::Upgrade)
        {
            const int32 Cue = Command.type == wildlight::CommandType::Build ? 1 : Command.type == wildlight::CommandType::CastSpell ? 2 : 0;
            PlayCue(Cue, ToWorld(Command.position), false);
        }
    }
    else PlayCue(3, FVector::ZeroVector, false);
    return Result;
}
wildlight::CommandResult AWildlightWorld::SetAutomaticGrowthPolicy(const wildlight::AutomaticGrowthPolicy& Policy)
{
    wildlight::Command Command;
    Command.type = wildlight::CommandType::SetAutomaticGrowth;
    Command.owner = wildlight::Owner::Player;
    Command.automatic_growth = Policy;
    return Submit(Command);
}
void AWildlightWorld::UpdatePreview(bool bVisible, wildlight::BuildingKind Kind, wildlight::Vec2 Position, uint8 Rotation)
{
    if (!Preview) return;
    Preview->SetVisibility(bVisible);
    if (!bVisible) return;
    const auto& D = wildlight::definition(Kind);
    Preview->SetWorldLocation(ToWorld(FootprintCenter(Kind, Position, Rotation), 12.f));
    Preview->SetWorldRotation(FRotator(0, Rotation * 90.f, 0));
    Preview->SetWorldScale3D(FVector(D.width * 2.f, D.height * 2.f, .15f));
    Preview->SetMaterial(0, Simulation.can_build(wildlight::Owner::Player, Kind, Position, Rotation).accepted ? ValidMaterial : InvalidMaterial);
}
bool AWildlightWorld::SaveMatch(const FString& Slot, FString& Error) const
{
    const FString Path = SavePath(Slot), Temporary = Path + TEXT(".tmp");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
    const std::string Bytes = Simulation.serialize();
    const FString Text = UTF8_TO_TCHAR(Bytes.c_str());
    if (!FFileHelper::SaveStringToFile(Text, *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) { Error = TEXT("Could not write the save. Check available storage."); return false; }
    if (!IFileManager::Get().Move(*Path, *Temporary, true, true, false, false)) { Error = TEXT("Could not replace the save safely."); return false; }
    return true;
}
bool AWildlightWorld::LoadMatch(const FString& Slot, FString& Error)
{
    const FString Path = SavePath(Slot);
    const int64 Size = IFileManager::Get().FileSize(*Path);
    if (Size <= 0 || Size > 8 * 1024 * 1024) { Error = TEXT("Save missing or too large."); return false; }
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *Path)) { Error = TEXT("Could not read the save."); return false; }
    const auto Result = Simulation.deserialize(TCHAR_TO_UTF8(*Text));
    if (!Result.accepted) { Error = UTF8_TO_TCHAR(Result.reason.c_str()); return false; }
    for (auto& Pair : Visuals) if (Pair.Value) Pair.Value->Destroy();
    Visuals.Empty(); Selection.Empty();
    PreviousHealth.Empty(); PresentedBuildingLevels.Empty();
    InitializeEventCursorFromSave();
    PreviousOutcome = Simulation.state().outcome;
    bRunning = true; bPaused = false;
    BuildArena(); RefreshPresentation();
    return true;
}
void AWildlightWorld::PlayCue(int32 Index, FVector Position, bool bSpatial)
{
    if (!Sounds.IsValidIndex(Index) || !Sounds[Index]) return;
    const auto* Preferences = Cast<UWildlightGameInstance>(GetGameInstance());
    const float Volume = (Preferences ? Preferences->MasterVolume : .65f) * (Index == 4 ? .35f : .55f);
    if (bSpatial) UGameplayStatics::PlaySoundAtLocation(this, Sounds[Index], Position, Volume);
    else UGameplayStatics::PlaySound2D(this, Sounds[Index], Volume);
}
void AWildlightWorld::PresentEvents()
{
    for (const auto& Event : Simulation.state().events)
    {
        if (Event.tick < LastEventTick) continue;
        if (Event.tick > LastEventTick) { LastEventTick = Event.tick; EventsAtLastTick.Empty(); }
        const FString Text = UTF8_TO_TCHAR(Event.text.c_str());
        const FString Key = EventPresentationKey(Event);
        if (EventsAtLastTick.Contains(Key)) continue;
        EventsAtLastTick.Add(Key);
        if (Event.owner != wildlight::Owner::Player || !bRunning) continue;
        if (Text.Contains(TEXT("completed")) || Text.Contains(TEXT("recruited")) || Text.Contains(TEXT("destroyed")) || Text.Contains(TEXT("upgraded")))
        {
            if (auto* Controller = Cast<AWildlightPlayerController>(UGameplayStatics::GetPlayerController(this, 0))) Controller->Notify(Text);
            PlayCue(Text.Contains(TEXT("destroyed")) ? 3 : 1, FVector::ZeroVector, false);
        }
    }
}
void AWildlightWorld::InitializeEventCursorFromSave()
{
    LastEventTick = Simulation.state().tick;
    EventsAtLastTick.Empty();
    for (const auto& Event : Simulation.state().events)
        if (Event.tick == LastEventTick) EventsAtLastTick.Add(EventPresentationKey(Event));
}
