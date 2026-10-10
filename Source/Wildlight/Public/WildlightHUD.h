#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "WildlightHUD.generated.h"

UCLASS()
class WILDLIGHT_API AWildlightHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    bool Click(FVector2D Position);
    bool IsOverHUD(FVector2D Position) const;
private:
    struct ButtonRegion { FBox2D Bounds; FString Action; FString Tooltip; bool bEnabled; };
    TArray<ButtonRegion> Buttons;
    TArray<FBox2D> Panels;
    int32 CatalogPage = 0;
    int32 ActionTab = 0;
    float Scale = 1.f;
    FLinearColor Ink = FLinearColor(0.91f, 0.94f, 0.84f);
    void Panel(float X, float Y, float W, float H, FLinearColor Color);
    void Label(const FString& Text, float X, float Y, float Size, FLinearColor Color);
    void Button(const FString& Text, const FString& Action, const FString& Tooltip, float X, float Y, float W, float H, bool bEnabled = true);
    void DrawMenus();
    void DrawMatch();
    void DrawMinimap(float X, float Y, float Size);
};
