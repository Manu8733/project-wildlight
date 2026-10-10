#include "WildlightGameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "UObject/UObjectIterator.h"

namespace
{
bool IsWildlightBindingKey(const FKey& Key)
{
    return Key.IsValid() && Key != EKeys::AnyKey && !Key.IsAxis1D() && !Key.IsAxis2D() && !Key.IsAxis3D();
}

TArray<FKey> WildlightEffectiveKeys(FName Role, const FKey& Key)
{
    TArray<FKey> Keys{Key};
    if (Role == TEXT("MultiSelect") && Key == EKeys::LeftShift) { Keys.Add(EKeys::RightShift); }
    if (Role == TEXT("GroupModifier") && Key == EKeys::LeftControl) { Keys.Add(EKeys::RightControl); }
    return Keys;
}
}

const TArray<FWildlightBindingDefinition>& UWildlightGameInstance::BindingCatalog()
{
    static const TArray<FWildlightBindingDefinition> Catalog{
        {TEXT("CameraForward"), TEXT("Pan forward"), EKeys::W, TEXT("CameraForward"), 1.f},
        {TEXT("CameraBackward"), TEXT("Pan backward"), EKeys::S, TEXT("CameraForward"), -1.f},
        {TEXT("CameraRight"), TEXT("Pan right"), EKeys::D, TEXT("CameraRight"), 1.f},
        {TEXT("CameraLeft"), TEXT("Pan left"), EKeys::A, TEXT("CameraRight"), -1.f},
        {TEXT("CameraRotateRight"), TEXT("Rotate clockwise"), EKeys::E, TEXT("CameraRotate"), 1.f},
        {TEXT("CameraRotateLeft"), TEXT("Rotate counterclockwise"), EKeys::Q, TEXT("CameraRotate"), -1.f},
        {TEXT("Select"), TEXT("Select / activate interface"), EKeys::LeftMouseButton, TEXT("Select"), 0.f},
        {TEXT("Context"), TEXT("Context order / cancel preview"), EKeys::RightMouseButton, TEXT("Context"), 0.f},
        {TEXT("Pause"), TEXT("Pause / back"), EKeys::Escape, TEXT("Pause"), 0.f},
        {TEXT("Build"), TEXT("Hearth Cottage shortcut"), EKeys::B, TEXT("Build"), 0.f},
        {TEXT("RotateBuilding"), TEXT("Rotate building preview"), EKeys::R, TEXT("RotateBuilding"), 0.f},
        {TEXT("Save"), TEXT("Quicksave"), EKeys::F5, TEXT("Save"), 0.f},
        {TEXT("Load"), TEXT("Quickload"), EKeys::F9, TEXT("Load"), 0.f},
        {TEXT("ZoomIn"), TEXT("Zoom in"), EKeys::MouseScrollUp, TEXT("ZoomIn"), 0.f},
        {TEXT("ZoomOut"), TEXT("Zoom out"), EKeys::MouseScrollDown, TEXT("ZoomOut"), 0.f},
        {TEXT("MultiSelect"), TEXT("Add to selection"), EKeys::LeftShift, NAME_None, 0.f},
        {TEXT("GroupModifier"), TEXT("Store control group"), EKeys::LeftControl, NAME_None, 0.f},
        {TEXT("Group1"), TEXT("Control group 1"), EKeys::One, NAME_None, 0.f},
        {TEXT("Group2"), TEXT("Control group 2"), EKeys::Two, NAME_None, 0.f},
        {TEXT("Group3"), TEXT("Control group 3"), EKeys::Three, NAME_None, 0.f},
        {TEXT("Group4"), TEXT("Control group 4"), EKeys::Four, NAME_None, 0.f},
        {TEXT("Group5"), TEXT("Control group 5"), EKeys::Five, NAME_None, 0.f},
        {TEXT("Group6"), TEXT("Control group 6"), EKeys::Six, NAME_None, 0.f},
        {TEXT("Group7"), TEXT("Control group 7"), EKeys::Seven, NAME_None, 0.f},
        {TEXT("Group8"), TEXT("Control group 8"), EKeys::Eight, NAME_None, 0.f},
        {TEXT("Group9"), TEXT("Control group 9"), EKeys::Nine, NAME_None, 0.f}
    };
    return Catalog;
}

FKey UWildlightGameInstance::KeyForBinding(FName Role) const
{
    if (const FKey* Key = KeyOverrides.Find(Role)) { return *Key; }
    for (const FWildlightBindingDefinition& Binding : BindingCatalog())
    {
        if (Binding.Role == Role) { return Binding.DefaultKey; }
    }
    return FKey();
}

bool UWildlightGameInstance::Rebind(FName Role, FKey Key, FString& Error)
{
    Error.Empty();
    const FWildlightBindingDefinition* Definition = nullptr;
    for (const FWildlightBindingDefinition& Binding : BindingCatalog())
    {
        if (Binding.Role == Role) { Definition = &Binding; break; }
    }
    if (!Definition || !IsWildlightBindingKey(Key))
    {
        Error = TEXT("Choose a keyboard key, mouse button, wheel direction, or digital controller button.");
        return false;
    }
    const TArray<FKey> NewKeys = WildlightEffectiveKeys(Role, Key);
    for (const FWildlightBindingDefinition& Other : BindingCatalog())
    {
        if (Other.Role == Role) { continue; }
        for (const FKey& UsedKey : WildlightEffectiveKeys(Other.Role, KeyForBinding(Other.Role)))
        {
            if (NewKeys.Contains(UsedKey))
            {
                Error = FString::Printf(TEXT("%s is already used for %s. Reassign that action first."), *UsedKey.GetDisplayName().ToString(), *Other.Label);
                return false;
            }
        }
    }
    KeyOverrides.Add(Role, Key);
    ApplyMappings();
    SavePreferences();
    return true;
}

void UWildlightGameInstance::ApplyMappings()
{
    UInputSettings* Settings = GetMutableDefault<UInputSettings>();
    TSet<FName> ActionNames;
    TSet<FName> AxisNames;
    for (const FWildlightBindingDefinition& Binding : BindingCatalog())
    {
        if (Binding.MappingName.IsNone()) { continue; }
        (Binding.AxisScale != 0.f ? AxisNames : ActionNames).Add(Binding.MappingName);
    }
    // Mutate only the in-memory defaults. Preferences live in our own config file.
    const TArray<FInputActionKeyMapping> OldActions = Settings->GetActionMappings();
    const TArray<FInputAxisKeyMapping> OldAxes = Settings->GetAxisMappings();
    for (const FInputActionKeyMapping& Mapping : OldActions)
    {
        if (ActionNames.Contains(Mapping.ActionName)) { Settings->RemoveActionMapping(Mapping, false); }
    }
    for (const FInputAxisKeyMapping& Mapping : OldAxes)
    {
        if (AxisNames.Contains(Mapping.AxisName)) { Settings->RemoveAxisMapping(Mapping, false); }
    }
    for (const FWildlightBindingDefinition& Binding : BindingCatalog())
    {
        if (Binding.MappingName.IsNone()) { continue; }
        const FKey Key = KeyForBinding(Binding.Role);
        if (Binding.AxisScale != 0.f) { Settings->AddAxisMapping(FInputAxisKeyMapping(Binding.MappingName, Key, Binding.AxisScale), false); }
        else { Settings->AddActionMapping(FInputActionKeyMapping(Binding.MappingName, Key, false, false, false, false), false); }
    }
    for (TObjectIterator<UPlayerInput> It; It; ++It)
    {
        It->ForceRebuildingKeyMaps(false);
    }
}

void UWildlightGameInstance::ResetBindings()
{
    KeyOverrides.Empty();
    ApplyMappings();
    SavePreferences();
}

void UWildlightGameInstance::Init()
{
    Super::Init();
    const FString Path = FPaths::GeneratedConfigDir() / TEXT("WildlightSettings.ini");
    GConfig->GetFloat(TEXT("Wildlight"), TEXT("UIScale"), UIScale, Path);
    GConfig->GetBool(TEXT("Wildlight"), TEXT("ReducedEffects"), bReducedEffects, Path);
    GConfig->GetBool(TEXT("Wildlight"), TEXT("EdgeScrolling"), bEdgeScrolling, Path);
    GConfig->GetInt(TEXT("Wildlight"), TEXT("GraphicsQuality"), GraphicsQuality, Path);
    GConfig->GetFloat(TEXT("Wildlight"), TEXT("MasterVolume"), MasterVolume, Path);
    MasterVolume = FMath::Clamp(MasterVolume, 0.f, 1.f);
    UIScale = FMath::Clamp(UIScale, 0.8f, 1.6f);
    KeyOverrides.Empty();
    TSet<FKey> LoadedKeys;
    bool bBindingsValid = true;
    for (const FWildlightBindingDefinition& Binding : BindingCatalog())
    {
        FString Stored;
        FKey Key = Binding.DefaultKey;
        if (GConfig->GetString(TEXT("Wildlight.Keys"), *Binding.Role.ToString(), Stored, Path))
        {
            const FKey StoredKey{FName(*Stored)};
            if (IsWildlightBindingKey(StoredKey)) { Key = StoredKey; }
        }
        KeyOverrides.Add(Binding.Role, Key);
        for (const FKey& EffectiveKey : WildlightEffectiveKeys(Binding.Role, Key))
        {
            if (LoadedKeys.Contains(EffectiveKey)) { bBindingsValid = false; }
            LoadedKeys.Add(EffectiveKey);
        }
    }
    if (!bBindingsValid) { KeyOverrides.Empty(); }
    ApplyMappings();
    ApplyGraphics(GraphicsQuality);
}
void UWildlightGameInstance::ApplyGraphics(int32 Quality)
{
    GraphicsQuality = FMath::Clamp(Quality, 0, 3);
    if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
    {
        Settings->SetOverallScalabilityLevel(GraphicsQuality);
        Settings->SetFrameRateLimit(60.f);
        Settings->ApplySettings(false);
    }
}
void UWildlightGameInstance::SavePreferences()
{
    const FString Path = FPaths::GeneratedConfigDir() / TEXT("WildlightSettings.ini");
    GConfig->SetFloat(TEXT("Wildlight"), TEXT("UIScale"), UIScale, Path);
    GConfig->SetBool(TEXT("Wildlight"), TEXT("ReducedEffects"), bReducedEffects, Path);
    GConfig->SetBool(TEXT("Wildlight"), TEXT("EdgeScrolling"), bEdgeScrolling, Path);
    GConfig->SetInt(TEXT("Wildlight"), TEXT("GraphicsQuality"), GraphicsQuality, Path);
    GConfig->SetFloat(TEXT("Wildlight"), TEXT("MasterVolume"), MasterVolume, Path);
    for (const FWildlightBindingDefinition& Binding : BindingCatalog())
    {
        GConfig->SetString(TEXT("Wildlight.Keys"), *Binding.Role.ToString(), *KeyForBinding(Binding.Role).GetFName().ToString(), Path);
    }
    GConfig->Flush(false, Path);
}
