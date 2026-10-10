#include "WildlightPlayerController.h"

#include "WildlightCameraPawn.h"
#include "WildlightGameInstance.h"
#include "WildlightHUD.h"
#include "WildlightWorld.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"

#include <cmath>

namespace
{
FString ControllerText(std::string_view Text)
{
    const std::string Copy(Text);
    return FString(UTF8_TO_TCHAR(Copy.c_str()));
}

TArray<uint32> ControllerSortedSelection(const AWildlightWorld& World)
{
    TArray<uint32> Ids = World.Selection.Array();
    Ids.Sort();
    return Ids;
}

bool ControllerOwnedEntity(const AWildlightWorld& World, uint32 Id)
{
    if (const wildlight::Unit* Unit = World.Simulation.unit(Id))
    {
        return Unit->owner == wildlight::Owner::Player;
    }
    if (const wildlight::Building* Building = World.Simulation.building(Id))
    {
        return Building->owner == wildlight::Owner::Player;
    }
    return false;
}

// Filter 0: all units; 1: gatherers; 2: construction/repair crews; 3: Weavers.
std::vector<wildlight::Id> ControllerSelectedUnits(const AWildlightWorld& World, int32 Filter = 0)
{
    std::vector<wildlight::Id> Result;
    for (uint32 Id : ControllerSortedSelection(World))
    {
        const wildlight::Unit* Unit = World.Simulation.unit(Id);
        if (!Unit || Unit->owner != wildlight::Owner::Player)
        {
            continue;
        }
        if (Filter == 1 && Unit->kind != wildlight::UnitKind::Worker)
        {
            continue;
        }
        if (Filter == 2 && Unit->kind != wildlight::UnitKind::Worker && Unit->kind != wildlight::UnitKind::Engineer)
        {
            continue;
        }
        if (Filter == 3 && Unit->kind != wildlight::UnitKind::Wizard)
        {
            continue;
        }
        Result.push_back(Id);
    }
    return Result;
}

wildlight::Id ControllerSelectedBuilding(const AWildlightWorld& World,
    wildlight::BuildingKind Kind = wildlight::BuildingKind::Count)
{
    return World.SelectedBuildingId(Kind);
}

bool ControllerMatchInput(const AWildlightPlayerController& Controller)
{
    return IsValid(Controller.Arena) && Controller.Arena->bRunning && !Controller.Arena->bPaused
        && !Controller.bMainMenu && !Controller.bSettings
        && Controller.Arena->Simulation.state().outcome == wildlight::MatchOutcome::Playing;
}

bool ControllerShift(const APlayerController& Controller)
{
    return Controller.IsInputKeyDown(EKeys::LeftShift) || Controller.IsInputKeyDown(EKeys::RightShift);
}

wildlight::Vec2 ControllerSnap(wildlight::Vec2 Position)
{
    return {std::floor(Position.x) + 0.5, std::floor(Position.y) + 0.5};
}

void ControllerReport(AWildlightPlayerController& Controller, const wildlight::CommandResult& Result,
    const FString& Success)
{
    Controller.Notify(Result.accepted ? Success : ControllerText(Result.reason));
}
}

void AWildlightPlayerController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    for (TActorIterator<AWildlightWorld> It(GetWorld()); It; ++It)
    {
        Arena = *It;
        break;
    }
}

void AWildlightPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    check(InputComponent);
    InputComponent->BindAction(TEXT("Select"), IE_Pressed, this, &AWildlightPlayerController::SelectPressed);
    InputComponent->BindAction(TEXT("Select"), IE_Released, this, &AWildlightPlayerController::SelectReleased);
    InputComponent->BindAction(TEXT("Context"), IE_Pressed, this, &AWildlightPlayerController::ContextPressed);
    InputComponent->BindAction(TEXT("Pause"), IE_Pressed, this, &AWildlightPlayerController::PausePressed);
    InputComponent->BindAction(TEXT("Build"), IE_Pressed, this, &AWildlightPlayerController::BuildPressed);
    InputComponent->BindAction(TEXT("RotateBuilding"), IE_Pressed, this, &AWildlightPlayerController::RotatePressed);
    InputComponent->BindAction(TEXT("Save"), IE_Pressed, this, &AWildlightPlayerController::SavePressed);
    InputComponent->BindAction(TEXT("Load"), IE_Pressed, this, &AWildlightPlayerController::LoadPressed);
    InputComponent->BindAction(TEXT("ZoomIn"), IE_Pressed, this, &AWildlightPlayerController::ZoomIn);
    InputComponent->BindAction(TEXT("ZoomOut"), IE_Pressed, this, &AWildlightPlayerController::ZoomOut);
    InputComponent->BindAxis(TEXT("CameraForward"));
    InputComponent->BindAxis(TEXT("CameraRight"));
    InputComponent->BindAxis(TEXT("CameraRotate"));
}

void AWildlightPlayerController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);
    if (!IsValid(Arena))
    {
        Arena = nullptr;
        for (TActorIterator<AWildlightWorld> It(GetWorld()); It; ++It)
        {
            Arena = *It;
            break;
        }
    }
    if (StatusSeconds > 0.f)
    {
        StatusSeconds = FMath::Max(0.f, StatusSeconds - DeltaSeconds);
        if (StatusSeconds <= 0.f)
        {
            Status.Empty();
        }
    }

    if (Arena)
    {
        for (auto It = Arena->Selection.CreateIterator(); It; ++It)
        {
            if (!ControllerOwnedEntity(*Arena, *It))
            {
                It.RemoveCurrent();
            }
        }
        if (Arena->bRunning && Arena->Simulation.state().outcome != wildlight::MatchOutcome::Playing)
        {
            Arena->bPaused = true;
            bDragging = bBuildPreview = bSpellPreview = bPatrolPreview = false;
        }
    }

    const bool bCanInteract = ControllerMatchInput(*this);
    AWildlightHUD* WildHUD = Cast<AWildlightHUD>(GetHUD());
    const bool bOverHUD = WildHUD && WildHUD->IsOverHUD(MousePosition());
    if (AWildlightCameraPawn* CameraPawn = Cast<AWildlightCameraPawn>(GetPawn()))
    {
        CameraPawn->bCameraInputEnabled = bCanInteract;
        const UWildlightGameInstance* Preferences = Cast<UWildlightGameInstance>(GetGameInstance());
        CameraPawn->bEdgeScrollingEnabled = (!Preferences || Preferences->bEdgeScrolling) && !bOverHUD && !bDragging;
        CameraPawn->MoveCamera(bCanInteract ? GetInputAxisValue(TEXT("CameraForward")) : 0.f,
            bCanInteract ? GetInputAxisValue(TEXT("CameraRight")) : 0.f);
        CameraPawn->RotateCamera(bCanInteract ? GetInputAxisValue(TEXT("CameraRotate")) : 0.f);
    }

    if (Arena)
    {
        wildlight::Vec2 Position;
        const bool bShowPreview = bCanInteract && bBuildPreview && !bOverHUD && GroundPosition(Position);
        Arena->UpdatePreview(bShowPreview, BuildKind, bShowPreview ? ControllerSnap(Position) : wildlight::Vec2{}, BuildRotation);
    }
    if (bCanInteract)
    {
        const FKey GroupKeys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
            EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
        for (int32 Index = 0; Index < 9; ++Index)
        {
            if (WasInputKeyJustPressed(GroupKeys[Index]))
            {
                RecallGroup(Index);
            }
        }
    }
}

FVector2D AWildlightPlayerController::MousePosition() const
{
    float X = -1.f;
    float Y = -1.f;
    GetMousePosition(X, Y);
    return FVector2D(X, Y);
}

bool AWildlightPlayerController::GroundPosition(wildlight::Vec2& Out) const
{
    Out = {};
    const AWildlightCameraPawn* CameraPawn = Cast<AWildlightCameraPawn>(GetPawn());
    FVector Ground;
    if (!Arena || !CameraPawn || !CameraPawn->GetGroundTarget(Ground))
    {
        return false;
    }
    Out = Arena->ToCell(Ground);
    const wildlight::State& State = Arena->Simulation.state();
    return Out.x >= 0 && Out.y >= 0 && Out.x < State.width && Out.y < State.height;
}

void AWildlightPlayerController::SelectPressed()
{
    bDragging = false;
    bUIConsumed = false;
    const FVector2D Mouse = MousePosition();
    if (AWildlightHUD* WildHUD = Cast<AWildlightHUD>(GetHUD()))
    {
        bUIConsumed = WildHUD->Click(Mouse);
        if (bUIConsumed || WildHUD->IsOverHUD(Mouse))
        {
            bUIConsumed = true;
            return;
        }
    }
    if (!ControllerMatchInput(*this))
    {
        return;
    }
    wildlight::Vec2 Position;
    if (!GroundPosition(Position))
    {
        return;
    }

    if (bBuildPreview)
    {
        bUIConsumed = true;
        wildlight::Command Command;
        Command.type = wildlight::CommandType::Build;
        Command.building = BuildKind;
        Command.position = ControllerSnap(Position);
        Command.rotation = BuildRotation;
        Command.ids = ControllerSelectedUnits(*Arena);
        const wildlight::CommandResult Result = Arena->Submit(Command);
        ControllerReport(*this, Result, ControllerText(wildlight::definition(BuildKind).name) + TEXT(" foundation placed."));
        if (Result.accepted && !ControllerShift(*this))
        {
            bBuildPreview = false;
            Arena->UpdatePreview(false, BuildKind, Position, BuildRotation);
        }
        return;
    }
    if (bSpellPreview)
    {
        bUIConsumed = true;
        wildlight::Command Command;
        Command.type = wildlight::CommandType::CastSpell;
        Command.spell = SpellKind;
        Command.position = Position;
        Command.target_id = Arena->Pick(Position, false);
        const std::vector<wildlight::Id> Casters = ControllerSelectedUnits(*Arena, 3);
        wildlight::Id CasterId = 0;
        double BestDistance = TNumericLimits<double>::Max();
        bool bBestReady = false;
        const double RangeSquared = FMath::Square(wildlight::definition(SpellKind).cast_range);
        for (wildlight::Id Id : Casters)
        {
            const wildlight::Unit* Unit = Arena->Simulation.unit(Id);
            const double Distance = FMath::Square(Unit->position.x - Position.x) + FMath::Square(Unit->position.y - Position.y);
            const bool bReady = Unit->spell_cooldowns[static_cast<size_t>(SpellKind)] <= 0 && Distance <= RangeSquared;
            if (!CasterId || (bReady && !bBestReady) || (bReady == bBestReady && Distance < BestDistance))
            {
                CasterId = Id;
                BestDistance = Distance;
                bBestReady = bReady;
            }
        }
        if (!CasterId)
        {
            Notify(TEXT("Select a Wildlight Weaver before casting."));
            return;
        }
        Command.ids.push_back(CasterId);
        const wildlight::CommandResult Result = Arena->Submit(Command);
        ControllerReport(*this, Result, ControllerText(wildlight::definition(SpellKind).name) + TEXT(" cast."));
        if (Result.accepted)
        {
            bSpellPreview = false;
        }
        return;
    }
    if (bPatrolPreview)
    {
        bUIConsumed = true;
        wildlight::Command Command;
        Command.type = wildlight::CommandType::Patrol;
        Command.position = Position;
        Submit(Command);
        bPatrolPreview = false;
        return;
    }
    DragStart = Mouse;
    bDragging = true;
}

void AWildlightPlayerController::SelectReleased()
{
    if (bUIConsumed)
    {
        bUIConsumed = false;
        bDragging = false;
        return;
    }
    if (!bDragging)
    {
        return;
    }
    bDragging = false;
    if (!ControllerMatchInput(*this))
    {
        return;
    }
    const FVector2D Mouse = MousePosition();
    if (Mouse.X < 0 || Mouse.Y < 0)
    {
        return;
    }
    if (!ControllerShift(*this))
    {
        Arena->Selection.Reset();
    }
    if (FVector2D::DistSquared(DragStart, Mouse) <= 36.0)
    {
        if (const AWildlightHUD* WildHUD = Cast<AWildlightHUD>(GetHUD()); WildHUD && WildHUD->IsOverHUD(Mouse))
        {
            Arena->RefreshPresentation();
            return;
        }
        wildlight::Vec2 Position;
        if (GroundPosition(Position))
        {
            const wildlight::Id Id = Arena->Pick(Position, false);
            if (ControllerOwnedEntity(*Arena, Id))
            {
                Arena->Selection.Add(Id);
            }
        }
    }
    else
    {
        const FVector2D Minimum(FMath::Min(DragStart.X, Mouse.X), FMath::Min(DragStart.Y, Mouse.Y));
        const FVector2D Maximum(FMath::Max(DragStart.X, Mouse.X), FMath::Max(DragStart.Y, Mouse.Y));
        for (const wildlight::Unit& Unit : Arena->Simulation.state().units)
        {
            if (Unit.owner != wildlight::Owner::Player || Unit.health <= 0)
            {
                continue;
            }
            FVector2D Screen;
            if (ProjectWorldLocationToScreen(Arena->ToWorld(Unit.position, 80.f), Screen, true)
                && Screen.X >= Minimum.X && Screen.X <= Maximum.X
                && Screen.Y >= Minimum.Y && Screen.Y <= Maximum.Y)
            {
                Arena->Selection.Add(Unit.id);
            }
        }
    }
    Arena->RefreshPresentation();
}

void AWildlightPlayerController::ContextPressed()
{
    if (bBuildPreview || bSpellPreview || bPatrolPreview)
    {
        bBuildPreview = bSpellPreview = bPatrolPreview = false;
        if (Arena)
        {
            Arena->UpdatePreview(false, BuildKind, {}, BuildRotation);
        }
        return;
    }
    if (!ControllerMatchInput(*this))
    {
        return;
    }
    if (const AWildlightHUD* WildHUD = Cast<AWildlightHUD>(GetHUD()); WildHUD && WildHUD->IsOverHUD(MousePosition()))
    {
        return;
    }
    wildlight::Vec2 Position;
    if (!GroundPosition(Position))
    {
        return;
    }
    const std::vector<wildlight::Id> Units = ControllerSelectedUnits(*Arena);
    if (Units.empty())
    {
        bool bRallied = false;
        for (uint32 Id : ControllerSortedSelection(*Arena))
        {
            const wildlight::Building* Building = Arena->Simulation.building(Id);
            if (Building && Building->owner == wildlight::Owner::Player)
            {
                wildlight::Command Rally;
                Rally.type = wildlight::CommandType::SetRally;
                Rally.target_id = Id;
                Rally.position = Position;
                Submit(Rally);
                bRallied = true;
            }
        }
        if (!bRallied)
        {
            Notify(TEXT("Select units or a recruiting building first."));
        }
        return;
    }

    wildlight::Command Command;
    Command.type = wildlight::CommandType::Move;
    Command.position = Position;
    Command.ids = Units;
    const wildlight::Id TargetId = Arena->Pick(Position, true);
    const wildlight::Unit* TargetUnit = Arena->Simulation.unit(TargetId);
    const wildlight::Building* TargetBuilding = Arena->Simulation.building(TargetId);
    const wildlight::ResourceNode* TargetNode = Arena->Simulation.node(TargetId);
    if ((TargetUnit && TargetUnit->owner == wildlight::Owner::Enemy)
        || (TargetBuilding && TargetBuilding->owner == wildlight::Owner::Enemy))
    {
        const wildlight::Vec2 TargetPosition = TargetUnit ? TargetUnit->position : TargetBuilding->position;
        if (Arena->Simulation.is_visible(wildlight::Owner::Player, TargetPosition))
        {
            Command.type = wildlight::CommandType::Attack;
            Command.target_id = TargetId;
        }
    }
    else if ((TargetNode && TargetNode->remaining > 0)
        || (TargetBuilding && TargetBuilding->owner == wildlight::Owner::Player
            && TargetBuilding->kind == wildlight::BuildingKind::Farm && TargetBuilding->progress >= 1
            && TargetBuilding->health >= wildlight::definition(TargetBuilding->kind).health * (1.0 + 0.35 * (TargetBuilding->level - 1)) - 0.01))
    {
        const std::vector<wildlight::Id> Workers = ControllerSelectedUnits(*Arena, 1);
        if (!Workers.empty())
        {
            Command.type = wildlight::CommandType::Gather;
            Command.target_id = TargetId;
            Command.ids = Workers;
        }
    }
    else if (TargetBuilding && TargetBuilding->owner == wildlight::Owner::Player
        && (TargetBuilding->progress < 1 || TargetBuilding->health < wildlight::definition(TargetBuilding->kind).health * (1.0 + 0.35 * (TargetBuilding->level - 1)) - 0.01))
    {
        const std::vector<wildlight::Id> Builders = ControllerSelectedUnits(*Arena, 2);
        if (!Builders.empty())
        {
            Command.type = wildlight::CommandType::Repair;
            Command.target_id = TargetId;
            Command.ids = Builders;
        }
    }
    Submit(Command);
}

void AWildlightPlayerController::PausePressed()
{
    if (bBuildPreview || bSpellPreview || bPatrolPreview || bDragging)
    {
        bBuildPreview = bSpellPreview = bPatrolPreview = bDragging = false;
        if (Arena)
        {
            Arena->UpdatePreview(false, BuildKind, {}, BuildRotation);
        }
        return;
    }
    if (bSettings)
    {
        bSettings = false;
        return;
    }
    if (!bMainMenu && Arena && Arena->bRunning && Arena->Simulation.state().outcome == wildlight::MatchOutcome::Playing)
    {
        Arena->bPaused = !Arena->bPaused;
    }
}

void AWildlightPlayerController::BuildPressed()
{
    if (ControllerMatchInput(*this))
    {
        bBuildPreview = !bBuildPreview;
        bSpellPreview = bPatrolPreview = false;
        BuildKind = wildlight::BuildingKind::House;
        BuildRotation = 0;
        if (bBuildPreview)
        {
            Notify(TEXT("Place a Hearth Cottage. R rotates; right-click or Esc cancels. Hold Shift to place several."));
        }
    }
}

void AWildlightPlayerController::RotatePressed()
{
    if (ControllerMatchInput(*this) && bBuildPreview)
    {
        BuildRotation = (BuildRotation + 1) % 4;
    }
}

void AWildlightPlayerController::ZoomIn()
{
    if (AWildlightCameraPawn* CameraPawn = Cast<AWildlightCameraPawn>(GetPawn()); CameraPawn && ControllerMatchInput(*this))
    {
        CameraPawn->ZoomCamera(1.f);
    }
}

void AWildlightPlayerController::ZoomOut()
{
    if (AWildlightCameraPawn* CameraPawn = Cast<AWildlightCameraPawn>(GetPawn()); CameraPawn && ControllerMatchInput(*this))
    {
        CameraPawn->ZoomCamera(-1.f);
    }
}

void AWildlightPlayerController::StartNewGame(bool bTutorial)
{
    if (!Arena)
    {
        Notify(TEXT("The world is still loading. Try again in a moment."));
        return;
    }
    MapChoice = FMath::Clamp(MapChoice, 0, 2);
    PersonalityChoice = FMath::Clamp(PersonalityChoice, 0, 2);
    DifficultyChoice = FMath::Clamp(DifficultyChoice, 0, 2);
    Arena->StartMatch(MapChoice, static_cast<wildlight::AIPersonality>(PersonalityChoice),
        static_cast<wildlight::Difficulty>(DifficultyChoice));
    Arena->bTutorial = bTutorial;
    Arena->bPaused = false;
    Arena->Selection.Reset();
    bMainMenu = bSettings = bBuildPreview = bSpellPreview = bPatrolPreview = bDragging = bUIConsumed = false;
    BuildRotation = 0;
    for (TArray<uint32>& Group : Groups)
    {
        Group.Reset();
    }
    LastGroup = -1;
    LastGroupTime = -100.f;
    if (AWildlightCameraPawn* CameraPawn = Cast<AWildlightCameraPawn>(GetPawn()))
    {
        const wildlight::State& State = Arena->Simulation.state();
        CameraPawn->SetWorldBounds(FVector2D(State.width * AWildlightWorld::CellSize, State.height * AWildlightWorld::CellSize));
        CameraPawn->FocusAt(Arena->ToWorld(wildlight::map_catalog()[MapChoice].player_start));
        CameraPawn->bCameraInputEnabled = true;
    }
    Arena->RefreshPresentation();
    Notify(bTutorial ? TEXT("Welcome to Wildlight. Follow the guide above the command panel.")
        : TEXT("Build your kingdom, connect Threadlight, and control the ancient beacons."));
}

void AWildlightPlayerController::SavePressed()
{
    if (!Arena || !Arena->bRunning)
    {
        Notify(TEXT("Start a match before saving."));
        return;
    }
    FString Error;
    Notify(Arena->SaveMatch(TEXT("Quicksave"), Error) ? TEXT("Quicksave saved.") : Error);
}

void AWildlightPlayerController::LoadPressed()
{
    if (!Arena)
    {
        Notify(TEXT("The world is still loading."));
        return;
    }
    FString Error;
    if (!Arena->LoadMatch(TEXT("Quicksave"), Error))
    {
        Notify(Error);
        return;
    }
    Arena->Selection.Reset();
    Arena->bPaused = false;
    Arena->bTutorial = false;
    bMainMenu = bSettings = bBuildPreview = bSpellPreview = bPatrolPreview = bDragging = bUIConsumed = false;
    for (TArray<uint32>& Group : Groups)
    {
        Group.Reset();
    }
    LastGroup = -1;
    LastGroupTime = -100.f;
    const wildlight::State& State = Arena->Simulation.state();
    MapChoice = FMath::Clamp(static_cast<int32>(State.map_index), 0, 2);
    PersonalityChoice = static_cast<int32>(State.ai_personality);
    DifficultyChoice = static_cast<int32>(State.difficulty);
    if (AWildlightCameraPawn* CameraPawn = Cast<AWildlightCameraPawn>(GetPawn()))
    {
        CameraPawn->SetWorldBounds(FVector2D(State.width * AWildlightWorld::CellSize, State.height * AWildlightWorld::CellSize));
        CameraPawn->FocusAt(Arena->ToWorld(wildlight::map_catalog()[MapChoice].player_start));
    }
    Arena->UpdatePreview(false, BuildKind, {}, BuildRotation);
    Arena->RefreshPresentation();
    Notify(TEXT("Quicksave loaded."));
}

void AWildlightPlayerController::Notify(const FString& Text)
{
    Status = Text;
    StatusSeconds = 6.f;
}

void AWildlightPlayerController::Submit(wildlight::Command Command)
{
    if (!ControllerMatchInput(*this))
    {
        return;
    }
    Command.owner = wildlight::Owner::Player;
    if (Command.ids.empty())
    {
        const int32 Filter = Command.type == wildlight::CommandType::Gather ? 1
            : Command.type == wildlight::CommandType::Repair ? 2 : 0;
        Command.ids = ControllerSelectedUnits(*Arena, Filter);
    }
    const wildlight::CommandResult Result = Arena->Submit(Command);
    FString Success = TEXT("Order issued.");
    switch (Command.type)
    {
    case wildlight::CommandType::Train: Success = ControllerText(wildlight::definition(Command.unit).name) + TEXT(" added to the recruitment queue."); break;
    case wildlight::CommandType::SetRally: Success = TEXT("Rally point set."); break;
    case wildlight::CommandType::Research: Success = TEXT("Research started."); break;
    case wildlight::CommandType::Trade: Success = TEXT("Trade completed."); break;
    case wildlight::CommandType::Upgrade: Success = TEXT("Building upgraded."); break;
    case wildlight::CommandType::Demolish: Success = TEXT("Building dismantled; recoverable supplies returned."); break;
    case wildlight::CommandType::Repair: Success = TEXT("Construction crew assigned."); break;
    case wildlight::CommandType::Gather: Success = TEXT("Hearthfolk assigned to gather."); break;
    case wildlight::CommandType::Patrol: Success = TEXT("Patrol route set."); break;
    case wildlight::CommandType::SetStance: Success = TEXT("Unit stance changed."); break;
    default: break;
    }
    ControllerReport(*this, Result, Success);
}

void AWildlightPlayerController::RecallGroup(int32 Index)
{
    if (!ControllerMatchInput(*this) || Index < 0 || Index >= 9)
    {
        return;
    }
    if (IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl))
    {
        Groups[Index] = ControllerSortedSelection(*Arena);
        LastGroup = -1;
        Notify(FString::Printf(TEXT("Control group %d saved (%d selected)."), Index + 1, Groups[Index].Num()));
        return;
    }
    const float Now = GetWorld()->GetTimeSeconds();
    const bool bFocus = LastGroup == Index && Now - LastGroupTime <= 0.35f;
    LastGroup = Index;
    LastGroupTime = Now;
    if (!ControllerShift(*this))
    {
        Arena->Selection.Reset();
    }
    FVector Centre = FVector::ZeroVector;
    int32 Count = 0;
    for (uint32 Id : Groups[Index])
    {
        if (!ControllerOwnedEntity(*Arena, Id))
        {
            continue;
        }
        Arena->Selection.Add(Id);
        if (const wildlight::Unit* Unit = Arena->Simulation.unit(Id))
        {
            Centre += Arena->ToWorld(Unit->position);
            ++Count;
        }
        else if (const wildlight::Building* Building = Arena->Simulation.building(Id))
        {
            Centre += Arena->ToWorld(Building->position);
            ++Count;
        }
    }
    if (bFocus && Count > 0)
    {
        if (AWildlightCameraPawn* CameraPawn = Cast<AWildlightCameraPawn>(GetPawn()))
        {
            CameraPawn->FocusAt(Centre / Count);
        }
    }
    Arena->RefreshPresentation();
}

void AWildlightPlayerController::HandleUIAction(const FString& Action)
{
    if (Action == TEXT("start")) { StartNewGame(false); return; }
    if (Action == TEXT("tutorial")) { StartNewGame(true); return; }
    if (Action == TEXT("mapnext")) { MapChoice = (MapChoice + 1) % 3; return; }
    if (Action == TEXT("ainext")) { PersonalityChoice = (PersonalityChoice + 1) % 3; return; }
    if (Action == TEXT("difficultynext")) { DifficultyChoice = (DifficultyChoice + 1) % 3; return; }
    if (Action == TEXT("settings"))
    {
        bSettings = true;
        bDragging = false;
        if (Arena && Arena->bRunning) { Arena->bPaused = true; }
        return;
    }
    if (Action == TEXT("back")) { bSettings = false; return; }
    if (Action == TEXT("resume"))
    {
        if (Arena && Arena->bRunning && Arena->Simulation.state().outcome == wildlight::MatchOutcome::Playing)
        {
            bMainMenu = bSettings = false;
            Arena->bPaused = false;
        }
        return;
    }
    if (Action == TEXT("restart")) { StartNewGame(Arena && Arena->bTutorial); return; }
    if (Action == TEXT("menu"))
    {
        bMainMenu = true;
        bSettings = bBuildPreview = bSpellPreview = bPatrolPreview = bDragging = false;
        if (Arena) { Arena->bPaused = true; Arena->UpdatePreview(false, BuildKind, {}, BuildRotation); }
        return;
    }
    if (Action == TEXT("save")) { SavePressed(); return; }
    if (Action == TEXT("load")) { LoadPressed(); return; }
    if (Action == TEXT("pause")) { PausePressed(); return; }
    if (Action == TEXT("quit")) { UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false); return; }

    if (UWildlightGameInstance* Preferences = Cast<UWildlightGameInstance>(GetGameInstance()))
    {
        bool bChanged = true;
        if (Action == TEXT("uiscale+") || Action == TEXT("scaleup")) { Preferences->UIScale = FMath::Clamp(Preferences->UIScale + 0.1f, 0.8f, 1.6f); }
        else if (Action == TEXT("uiscale-") || Action == TEXT("scaledown")) { Preferences->UIScale = FMath::Clamp(Preferences->UIScale - 0.1f, 0.8f, 1.6f); }
        else if (Action == TEXT("qualitynext")) { Preferences->ApplyGraphics((Preferences->GraphicsQuality + 1) % 4); }
        else if (Action == TEXT("effects")) { Preferences->bReducedEffects = !Preferences->bReducedEffects; }
        else if (Action == TEXT("edges")) { Preferences->bEdgeScrolling = !Preferences->bEdgeScrolling; }
        else if (Action == TEXT("volume"))
        {
            const float NextVolume = Preferences->MasterVolume + 0.25f;
            Preferences->MasterVolume = NextVolume > 1.f ? 0.f : NextVolume;
        }
        else { bChanged = false; }
        if (bChanged) { Preferences->SavePreferences(); return; }
    }
    if (!ControllerMatchInput(*this))
    {
        return;
    }

    if (Action.StartsWith(TEXT("focus:")))
    {
        TArray<FString> Coordinates;
        Action.ParseIntoArray(Coordinates, TEXT(":"), false);
        if (Coordinates.Num() == 3)
        {
            const double X = FCString::Atod(*Coordinates[1]);
            const double Y = FCString::Atod(*Coordinates[2]);
            const wildlight::State& State = Arena->Simulation.state();
            if (FMath::IsFinite(X) && FMath::IsFinite(Y))
            {
                if (AWildlightCameraPawn* CameraPawn = Cast<AWildlightCameraPawn>(GetPawn()))
                {
                    const wildlight::Vec2 Position{FMath::Clamp(X, 0.5, State.width - 0.5),
                        FMath::Clamp(Y, 0.5, State.height - 0.5)};
                    CameraPawn->FocusAt(Arena->ToWorld(Position));
                }
            }
        }
        return;
    }

    if (Action.StartsWith(TEXT("build:")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(6));
        if (Index >= 0 && Index < static_cast<int32>(wildlight::BuildingKind::Count))
        {
            BuildKind = static_cast<wildlight::BuildingKind>(Index);
            BuildRotation = 0;
            bBuildPreview = true;
            bSpellPreview = bPatrolPreview = false;
            Notify(TEXT("Choose a site. R rotates; right-click or Esc cancels. Shift places several."));
        }
        return;
    }
    if (Action.StartsWith(TEXT("spell:")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(6));
        if (Index >= 0 && Index < static_cast<int32>(wildlight::SpellKind::Count))
        {
            if (ControllerSelectedUnits(*Arena, 3).empty())
            {
                Notify(TEXT("Select a Wildlight Weaver first."));
                return;
            }
            SpellKind = static_cast<wildlight::SpellKind>(Index);
            bSpellPreview = true;
            bBuildPreview = bPatrolPreview = false;
            Notify(TEXT("Choose the spell target. Right-click or Esc cancels."));
        }
        return;
    }

    wildlight::Command Command;
    if (Action.StartsWith(TEXT("train:")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(6));
        if (Index < 0 || Index >= static_cast<int32>(wildlight::UnitKind::Count)) { return; }
        Command.type = wildlight::CommandType::Train;
        Command.unit = static_cast<wildlight::UnitKind>(Index);
        Command.target_id = ControllerSelectedBuilding(*Arena, wildlight::definition(Command.unit).recruited_at);
        if (!Command.target_id) { Notify(TEXT("Select the building that recruits this unit.")); return; }
        Command.ids.push_back(Command.target_id);
    }
    else if (Action.StartsWith(TEXT("research:")))
    {
        const int32 Index = FCString::Atoi(*Action.Mid(9));
        if (Index < 0 || Index >= static_cast<int32>(wildlight::Technology::Count)) { return; }
        Command.type = wildlight::CommandType::Research;
        Command.technology = static_cast<wildlight::Technology>(Index);
    }
    else if (Action == TEXT("repair"))
    {
        Command.type = wildlight::CommandType::Repair;
        Command.target_id = ControllerSelectedBuilding(*Arena);
        if (!Command.target_id) { Notify(TEXT("Select a building and Hearthfolk or Runesmiths to repair it.")); return; }
    }
    else if (Action == TEXT("upgrade") || Action == TEXT("demolish"))
    {
        Command.type = Action == TEXT("upgrade") ? wildlight::CommandType::Upgrade : wildlight::CommandType::Demolish;
        Command.target_id = ControllerSelectedBuilding(*Arena);
        if (!Command.target_id) { Notify(TEXT("Select one of your buildings first.")); return; }
    }
    else if (Action == TEXT("stop")) { Command.type = wildlight::CommandType::Stop; }
    else if (Action == TEXT("patrol"))
    {
        if (ControllerSelectedUnits(*Arena).empty()) { Notify(TEXT("Select units to patrol.")); return; }
        bPatrolPreview = true;
        bBuildPreview = bSpellPreview = false;
        Notify(TEXT("Choose the other end of the patrol route. Right-click or Esc cancels."));
        return;
    }
    else if (Action == TEXT("stance"))
    {
        Command.type = wildlight::CommandType::SetStance;
        const std::vector<wildlight::Id> Units = ControllerSelectedUnits(*Arena);
        if (Units.empty()) { Notify(TEXT("Select units to change their stance.")); return; }
        const wildlight::Unit* Unit = Arena->Simulation.unit(Units.front());
        Command.stance = static_cast<wildlight::Stance>((static_cast<int32>(Unit->stance) + 1) % 3);
    }
    else if (Action == TEXT("trade"))
    {
        Command.type = wildlight::CommandType::Trade;
        Command.resource = wildlight::ResourceKind::Wood;
        Command.amount = 50;
    }
    else { return; }
    Submit(Command);
}

FString AWildlightPlayerController::TutorialText() const
{
    if (!Arena || !Arena->bRunning || !Arena->bTutorial)
    {
        return {};
    }
    const wildlight::State& State = Arena->Simulation.state();
    auto HasBuilding = [&State](wildlight::BuildingKind Kind, bool bRequireConnected = false)
    {
        for (const wildlight::Building& Building : State.buildings)
        {
            if (Building.owner == wildlight::Owner::Player && Building.kind == Kind
                && Building.health > 0 && Building.progress >= 1 && (!bRequireConnected || Building.connected))
            {
                return true;
            }
        }
        return false;
    };
    if (!HasBuilding(wildlight::BuildingKind::LumberCamp))
    {
        return TEXT("1 / 7 — Establish your economy: select Hearthfolk and right-click trees. Build a Timber Lodge near wood; wait until its construction finishes.");
    }
    if (!HasBuilding(wildlight::BuildingKind::Farm))
    {
        return TEXT("2 / 7 — Feed the kingdom: build a Sungrain Farm. Select Hearthfolk and right-click the completed farm to gather food.");
    }
    if (!HasBuilding(wildlight::BuildingKind::House))
    {
        return TEXT("3 / 7 — Make room for your people: build a Hearth Cottage. Its completed foundation increases your population capacity.");
    }
    if (!HasBuilding(wildlight::BuildingKind::Barracks))
    {
        return TEXT("4 / 7 — Raise defenders: build Sentinel Barracks, select them, and recruit Lantern Militia. Right-click ground to set their rally point.");
    }
    if (!HasBuilding(wildlight::BuildingKind::Beacon, true))
    {
        return TEXT("5 / 7 — Extend Threadlight: build a Threadlight Beacon close enough to connect to the Dawnkeep. Linked light enables magical buildings.");
    }
    if (!HasBuilding(wildlight::BuildingKind::MageTower, true))
    {
        return TEXT("6 / 7 — Bring magic to the kingdom: build a Starweaver Spire inside connected light. Select the completed Spire to recruit more Wildlight Weavers.");
    }
    for (const wildlight::Unit& Unit : State.units)
    {
        if (Unit.owner == wildlight::Owner::Player && Unit.kind == wildlight::UnitKind::Wizard && Unit.health > 0)
        {
            const wildlight::MapDefinition& Map = wildlight::map_catalog()[FMath::Clamp(static_cast<int32>(State.map_index), 0, 2)];
            if (Map.objective_beacons > 0)
            {
                return TEXT("7 / 7 — Select your Weaver, choose a spell, and click its target. Connect Threadlight Beacons at two ancient lantern sites and hold them for 90 seconds, or destroy the rival Dawnkeep.");
            }
            return TEXT("7 / 7 — Select your Weaver, choose a spell, and click its target. Explore, extend connected light, and lead your army to destroy the rival Dawnkeep. Keep your own Dawnkeep standing.");
        }
    }
    return TEXT("6 / 7 — Select your connected Starweaver Spire and recruit a Wildlight Weaver. Keep enough food, gold, mana, and population capacity available.");
}
