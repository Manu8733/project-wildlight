#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "WildlightGameInstance.generated.h"

struct FWildlightBindingDefinition
{
    FName Role;
    FString Label;
    FKey DefaultKey;
    FName MappingName;
    float AxisScale = 0.f;
};

UCLASS()
class WILDLIGHT_API UWildlightGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual void Init() override;
    void SavePreferences();
    void ApplyGraphics(int32 Quality);
    static const TArray<FWildlightBindingDefinition>& BindingCatalog();
    FKey KeyForBinding(FName Role) const;
    bool Rebind(FName Role, FKey Key, FString& Error);
    void ApplyMappings();
    void ResetBindings();
    float UIScale = 1.f;
    bool bReducedEffects = false;
    bool bEdgeScrolling = true;
    int32 GraphicsQuality = 0;
    float MasterVolume = 0.65f;
private:
    TMap<FName, FKey> KeyOverrides;
};
