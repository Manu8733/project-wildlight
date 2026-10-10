#include "WildlightGameMode.h"
#include "WildlightCameraPawn.h"
#include "WildlightPlayerController.h"
#include "WildlightHUD.h"
#include "WildlightWorld.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AWildlightGameMode::AWildlightGameMode()
{
    DefaultPawnClass = AWildlightCameraPawn::StaticClass();
    PlayerControllerClass = AWildlightPlayerController::StaticClass();
    HUDClass = AWildlightHUD::StaticClass();
}
void AWildlightGameMode::BeginPlay()
{
    Super::BeginPlay();
    if (TActorIterator<AWildlightWorld>(GetWorld())) return;
    GetWorld()->SpawnActor<AWildlightWorld>();
}
