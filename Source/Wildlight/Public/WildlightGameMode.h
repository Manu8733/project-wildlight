#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WildlightGameMode.generated.h"
UCLASS()
class WILDLIGHT_API AWildlightGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AWildlightGameMode();
    virtual void BeginPlay() override;
};
