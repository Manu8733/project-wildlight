#include "WildlightHUD.h"
#include "WildlightPlayerController.h"
#include "WildlightWorld.h"
#include "WildlightGameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "CanvasItem.h"
#include "Engine/Font.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCanvas.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Accessibility/SlateWidgetAccessibleTypes.h"

namespace
{
const FLinearColor PanelColor(.035f, .075f, .065f, .96f);
const FLinearColor Gold(.96f, .75f, .36f);
const FLinearColor Muted(.57f, .68f, .59f);
FString CostText(const wildlight::Resources& R)
{
    return FString::Printf(TEXT("Wood %.2f   Stone %.2f   Food %.2f   Gold %.2f   Light %.2f"), R.wood, R.stone, R.food, R.gold, R.mana);
}
FString Name(std::string_view S) { return UTF8_TO_TCHAR(std::string(S).c_str()); }
}
bool AWildlightHUD::CanEditAutomaticGrowth() const
{
    const auto* C = Cast<AWildlightPlayerController>(PlayerOwner);
    return C && IsValid(C->Arena) && C->Arena->bRunning && !C->Arena->bPaused
        && !C->bMainMenu && !C->bSettings
        && C->Arena->Simulation.state().outcome == wildlight::MatchOutcome::Playing;
}
wildlight::AutomaticGrowthPolicy AWildlightHUD::ReadAutomaticGrowthPolicy() const
{
    const auto* C = Cast<AWildlightPlayerController>(PlayerOwner);
    if (C && IsValid(C->Arena)) return C->Arena->Simulation.state().factions[0].automatic_growth;
    wildlight::AutomaticGrowthPolicy Policy;
    Policy.enabled = false;
    return Policy;
}
void AWildlightHUD::ApplyAutomaticGrowthPolicy(const wildlight::AutomaticGrowthPolicy& Policy)
{
    if (!CanEditAutomaticGrowth()) return;
    auto* C = Cast<AWildlightPlayerController>(PlayerOwner);
    const auto Result = C->Arena->SetAutomaticGrowthPolicy(Policy);
    C->Notify(Result.accepted ? TEXT("Automatic growth policy updated.") : UTF8_TO_TCHAR(Result.reason.c_str()));
}
FText AWildlightHUD::AutomaticGrowthStatusText() const
{
    const auto* C = Cast<AWildlightPlayerController>(PlayerOwner);
    if (!C || !IsValid(C->Arena)) return FText::GetEmpty();
    const auto* B = C->Arena->Simulation.building(C->Arena->SelectedBuildingId());
    if (!B || (B->kind != wildlight::BuildingKind::House && B->kind != wildlight::BuildingKind::Farm)) return FText::GetEmpty();
    const std::string Reason = C->Arena->Simulation.automatic_growth_status(B->id);
    return FText::FromString(FString::Printf(TEXT("Level %d / 3  |  Automatic growth: %s"), static_cast<int32>(B->level), UTF8_TO_TCHAR(Reason.c_str())));
}
void AWildlightHUD::EnsureGrowthControls()
{
    if (GrowthControls.IsValid() || !GetWorld()) return;
    UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
    if (!Viewport) return;
    const FText GrowthLabel = FText::FromString(TEXT("Automatic house and farm growth"));
    const FText WoodLabel = FText::FromString(TEXT("Protected wood reserve"));
    const FText StoneLabel = FText::FromString(TEXT("Protected stone reserve"));
    GrowthControls = SNew(SCanvas)
        .Visibility_Lambda([this]() { return CanEditAutomaticGrowth() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
        + SCanvas::Slot()
        .Position(TAttribute<FVector2D>::CreateLambda([this]() { return GrowthControlsPosition; }))
        .Size(TAttribute<FVector2D>::CreateLambda([this]() { return GrowthControlsSize; }))
        [
            SNew(SScaleBox)
            .Stretch(EStretch::UserSpecified)
            .UserSpecifiedScale_Lambda([this]() { return Scale; })
            .HAlign(HAlign_Left).VAlign(VAlign_Top)
            [
                SNew(SBox)
                .WidthOverride_Lambda([this]() { return GrowthControlsSize.X / Scale; })
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SCheckBox)
                        .IsFocusable(true)
                        .AccessibleParams(FAccessibleWidgetData(GrowthLabel, GrowthLabel, true))
                        .ToolTipText(FText::FromString(TEXT("Grow houses and farms automatically while keeping your protected reserves.")))
                        .IsChecked_Lambda([this]() { return ReadAutomaticGrowthPolicy().enabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
                        .OnCheckStateChanged_Lambda([this](ECheckBoxState State)
                        {
                            if (State == ECheckBoxState::Undetermined) return;
                            auto Policy = ReadAutomaticGrowthPolicy();
                            Policy.enabled = State == ECheckBoxState::Checked;
                            ApplyAutomaticGrowthPolicy(Policy);
                        })
                        [
                            SNew(STextBlock).Text(GrowthLabel)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14)).ColorAndOpacity(Ink)
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 12, 0)
                        [
                            SNew(SNumericEntryBox<double>)
                            .AllowSpin(false).AllowWheel(false)
                            .MinValue(0.0).MaxValue(1000000.0)
                            .MinFractionalDigits(0).MaxFractionalDigits(2).MinDesiredValueWidth(100)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
                            .AccessibleParams(FAccessibleWidgetData(WoodLabel, WoodLabel, true))
                            .ToolTipText(FText::FromString(TEXT("Wood kept after automatic growth. Reserve range: 0 to 1,000,000.")))
                            .Value_Lambda([this]() { return TOptional<double>(ReadAutomaticGrowthPolicy().protected_wood); })
                            .OnValueCommitted_Lambda([this](double Value, ETextCommit::Type CommitType)
                            {
                                if (CommitType == ETextCommit::OnCleared) return;
                                auto Policy = ReadAutomaticGrowthPolicy();
                                Policy.protected_wood = Value;
                                ApplyAutomaticGrowthPolicy(Policy);
                            })
                            .Label()
                            [
                                SNew(STextBlock).Text(FText::FromString(TEXT("Wood reserve")))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(Ink)
                            ]
                        ]
                        + SHorizontalBox::Slot().FillWidth(1)
                        [
                            SNew(SNumericEntryBox<double>)
                            .AllowSpin(false).AllowWheel(false)
                            .MinValue(0.0).MaxValue(1000000.0)
                            .MinFractionalDigits(0).MaxFractionalDigits(2).MinDesiredValueWidth(100)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
                            .AccessibleParams(FAccessibleWidgetData(StoneLabel, StoneLabel, true))
                            .ToolTipText(FText::FromString(TEXT("Stone kept after automatic growth. Reserve range: 0 to 1,000,000.")))
                            .Value_Lambda([this]() { return TOptional<double>(ReadAutomaticGrowthPolicy().protected_stone); })
                            .OnValueCommitted_Lambda([this](double Value, ETextCommit::Type CommitType)
                            {
                                if (CommitType == ETextCommit::OnCleared) return;
                                auto Policy = ReadAutomaticGrowthPolicy();
                                Policy.protected_stone = Value;
                                ApplyAutomaticGrowthPolicy(Policy);
                            })
                            .Label()
                            [
                                SNew(STextBlock).Text(FText::FromString(TEXT("Stone reserve")))
                                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(Ink)
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 0)
                    [
                        SNew(STextBlock).Text_Lambda([this]() { return AutomaticGrowthStatusText(); })
                        .AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(Gold)
                    ]
                ]
            ]
        ];
    GrowthViewport = Viewport;
    Viewport->AddViewportWidgetContent(GrowthControls.ToSharedRef(), 10);
}
void AWildlightHUD::EnsureActionTooltip()
{
    if (ActionTooltip.IsValid() || !GetWorld()) return;
    UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
    if (!Viewport) return;
    ActionTooltipContent = SNew(SBox)
        .WidthOverride_Lambda([this]() { return ActionTooltipWidth; })
        [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
            .BorderBackgroundColor(PanelColor).Padding(FMargin(10.f, 9.f))
            [
                SNew(STextBlock).Text_Lambda([this]() { return ActionTooltipText; })
                .WrapTextAt_Lambda([this]() { return FMath::Max(1.f, ActionTooltipWidth - 20.f); })
                .WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 12)).ColorAndOpacity(Ink)
            ]
        ];
    ActionTooltip = SNew(SCanvas)
        .Visibility_Lambda([this]() { return ActionTooltipText.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
        + SCanvas::Slot()
        .Position(TAttribute<FVector2D>::CreateLambda([this]() { return ActionTooltipPosition; }))
        .Size(TAttribute<FVector2D>::CreateLambda([this]() { return ActionTooltipSize; }))
        [
            SNew(SScaleBox).Stretch(EStretch::UserSpecified)
            .UserSpecifiedScale_Lambda([this]() { return ActionTooltipScale; })
            .HAlign(HAlign_Left).VAlign(VAlign_Top)
            [ ActionTooltipContent.ToSharedRef() ]
        ];
    TooltipViewport = Viewport;
    Viewport->AddViewportWidgetContent(ActionTooltip.ToSharedRef(), 20);
}
void AWildlightHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GrowthControls.IsValid() && GrowthViewport.IsValid()) GrowthViewport->RemoveViewportWidgetContent(GrowthControls.ToSharedRef());
    if (ActionTooltip.IsValid() && TooltipViewport.IsValid()) TooltipViewport->RemoveViewportWidgetContent(ActionTooltip.ToSharedRef());
    GrowthControls.Reset();
    GrowthViewport.Reset();
    ActionTooltip.Reset();
    ActionTooltipContent.Reset();
    TooltipViewport.Reset();
    Super::EndPlay(EndPlayReason);
}
void AWildlightHUD::Panel(float X, float Y, float W, float H, FLinearColor Color)
{
    FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
    Tile.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(Tile);
}
void AWildlightHUD::Label(const FString& Text, float X, float Y, float Size, FLinearColor Color)
{
    FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), GEngine->GetMediumFont(), Color);
    Item.Scale = FVector2D(Size * Scale / 16.f);
    Item.EnableShadow(FLinearColor(0, 0, 0, .65f));
    Canvas->DrawItem(Item);
}
void AWildlightHUD::Button(const FString& Text, const FString& Action, const FString& Tooltip, float X, float Y, float W, float H, bool bEnabled)
{
    const auto* Controller = Cast<AWildlightPlayerController>(PlayerOwner);
    const FBox2D Bounds(FVector2D(X, Y), FVector2D(X + W, Y + H));
    const bool bHovered = Controller && Bounds.IsInside(Controller->MousePosition());
    Panel(X, Y, W, H, bEnabled ? (bHovered ? FLinearColor(.24f, .34f, .23f, 1) : FLinearColor(.11f, .19f, .14f, 1)) : FLinearColor(.08f, .11f, .09f, .9f));
    if (bHovered && bEnabled) Panel(X, Y + H - 2.f * Scale, W, 2.f * Scale, Gold);
    Label(Text, X + 10.f * Scale, Y + (H - 18.f * Scale) * .5f, 14.f, bEnabled ? Ink : Muted);
    Buttons.Add({Bounds, Action, Tooltip, bEnabled});
}
bool AWildlightHUD::IsOverHUD(FVector2D Position) const
{
    for (const auto& Bounds : Panels) if (Bounds.IsInside(Position)) return true;
    return false;
}
bool AWildlightHUD::Click(FVector2D Position)
{
    for (const auto& B : Buttons)
    {
        if (!B.Bounds.IsInside(Position)) continue;
        if (!B.bEnabled) return true;
        if (B.Action == TEXT("page:next")) CatalogPage = (CatalogPage + 1) % 3;
        else if (B.Action == TEXT("page:prev")) CatalogPage = (CatalogPage + 2) % 3;
        else if (B.Action.StartsWith(TEXT("tab:"))) ActionTab = FMath::Clamp(FCString::Atoi(*B.Action.Mid(4)), 0, 3);
        else if (B.Action.StartsWith(TEXT("minimap:")))
        {
            TArray<FString> Parts;
            B.Action.ParseIntoArray(Parts, TEXT(":"));
            if (Parts.Num() == 5)
            {
                const float X = FCString::Atof(*Parts[1]), Y = FCString::Atof(*Parts[2]);
                const float W = FCString::Atof(*Parts[3]), H = FCString::Atof(*Parts[4]);
                if (auto* C = Cast<AWildlightPlayerController>(PlayerOwner))
                    C->HandleUIAction(FString::Printf(TEXT("focus:%.3f:%.3f"), (Position.X - B.Bounds.Min.X) / B.Bounds.GetSize().X * W + X, (Position.Y - B.Bounds.Min.Y) / B.Bounds.GetSize().Y * H + Y));
            }
        }
        else if (auto* C = Cast<AWildlightPlayerController>(PlayerOwner)) C->HandleUIAction(B.Action);
        return true;
    }
    return IsOverHUD(Position);
}
void AWildlightHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine) return;
    Buttons.Empty(); Panels.Empty();
    ActionTooltipText = FText::GetEmpty();
    const auto* Preferences = Cast<UWildlightGameInstance>(GetGameInstance());
    const float Requested = Preferences ? Preferences->UIScale : 1.f;
    // Menus keep their scale; the match also reserves space for guidance and growth.
    Scale = FMath::Clamp(Requested * FMath::Min(Canvas->ClipX / 1280.f, Canvas->ClipY / 720.f), .65f, 1.35f);
    auto* C = Cast<AWildlightPlayerController>(PlayerOwner);
    if (!C) return;
    if (C->bMainMenu || C->bSettings || (C->Arena && (C->Arena->bPaused || C->Arena->Simulation.state().outcome != wildlight::MatchOutcome::Playing))) DrawMenus();
    else DrawMatch();
    if (!C->Status.IsEmpty())
    {
        const float W = FMath::Min(800.f * Scale, Canvas->ClipX - 32.f);
        Panel((Canvas->ClipX - W) / 2, 76.f * Scale, W, 34.f * Scale, PanelColor);
        Label(C->Status.Left(95), (Canvas->ClipX - W) / 2 + 12.f * Scale, 84.f * Scale, 14, Gold);
    }
    const FVector2D Mouse = C->MousePosition();
    for (const auto& B : Buttons) if (B.Bounds.IsInside(Mouse) && !B.Tooltip.IsEmpty())
    {
        ActionTooltipText = FText::FromString(B.Tooltip);
        ActionTooltipWidth = FMath::Max(1.f, FMath::Min(630.f, (Canvas->ClipX - 24.f) / Scale));
        EnsureActionTooltip();
        if (ActionTooltipContent.IsValid())
        {
            // Measure wrapped text before positioning, including the first hover frame.
            ActionTooltipContent->Invalidate(EInvalidateWidgetReason::Layout);
            ActionTooltipContent->SlatePrepass(1.f);
            const auto DesiredSize = ActionTooltipContent->GetDesiredSize();
            ActionTooltipScale = FMath::Min(Scale, FMath::Max(1.f, Canvas->ClipY - 24.f) / FMath::Max(1.f, static_cast<float>(DesiredSize.Y)));
            ActionTooltipSize = FVector2D(DesiredSize.X * ActionTooltipScale, DesiredSize.Y * ActionTooltipScale);
            ActionTooltipPosition = FVector2D(
                FMath::Clamp(static_cast<float>(Mouse.X) + 14.f, 12.f, Canvas->ClipX - static_cast<float>(ActionTooltipSize.X) - 12.f),
                FMath::Clamp(static_cast<float>(Mouse.Y) - static_cast<float>(ActionTooltipSize.Y) - 16.f, 12.f, Canvas->ClipY - static_cast<float>(ActionTooltipSize.Y) - 12.f));
        }
        break;
    }
}
void AWildlightHUD::DrawMenus()
{
    auto* C = Cast<AWildlightPlayerController>(PlayerOwner);
    Panel(0, 0, Canvas->ClipX, Canvas->ClipY, FLinearColor(.02f, .04f, .035f, .60f));
    Panels.Add(FBox2D(FVector2D::ZeroVector, FVector2D(Canvas->ClipX, Canvas->ClipY)));
    const float W = FMath::Min(580.f * Scale, Canvas->ClipX - 40.f);
    const float X = (Canvas->ClipX - W) * .5f, Y = FMath::Max(24.f, (Canvas->ClipY - 540.f * Scale) * .5f);
    Panel(X, Y, W, 540.f * Scale, PanelColor);
    Panel(X, Y, W, 3.f * Scale, Gold);
    Label(TEXT("W I L D L I G H T"), X + 30.f * Scale, Y + 25.f * Scale, 32.f, Gold);
    Label(TEXT("Build a kingdom. Command magic."), X + 30.f * Scale, Y + 75.f * Scale, 16.f, Ink);
    const float BX = X + 30.f * Scale, BW = W - 60.f * Scale;
    float BY = Y + 122.f * Scale;
    auto Add = [&](const FString& Text, const FString& Action, const FString& Tip = FString())
    {
        Button(Text, Action, Tip, BX, BY, BW, 40.f * Scale);
        BY += 48.f * Scale;
    };
    if (C->bSettings)
    {
        const auto* P = Cast<UWildlightGameInstance>(GetGameInstance());
        const TCHAR* Quality[] = {TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic")};
        Add(FString::Printf(TEXT("Graphics: %s"), Quality[P ? P->GraphicsQuality : 0]), TEXT("qualitynext"), TEXT("Low is the starting preset for a 4 GB laptop GPU."));
        Add(FString::Printf(TEXT("Interface size: %.0f%%  (+)"), (P ? P->UIScale : 1.f) * 100), TEXT("uiscale+"));
        Add(TEXT("Smaller interface  (-)"), TEXT("uiscale-"));
        Add(FString::Printf(TEXT("Reduced effects: %s"), P && P->bReducedEffects ? TEXT("On") : TEXT("Off")), TEXT("effects"), TEXT("Disables ambient fireflies. Tactical spell effects remain visible."));
        Add(FString::Printf(TEXT("Edge scrolling: %s"), !P || P->bEdgeScrolling ? TEXT("On") : TEXT("Off")), TEXT("edges"));
        Add(FString::Printf(TEXT("Sound volume: %.0f%%"), (P ? P->MasterVolume : .65f) * 100), TEXT("volume"));
        Add(TEXT("Back"), TEXT("back"));
        Label(TEXT("WASD pan   Q/E rotate   Wheel zoom   Esc pause"), BX, BY + 5.f * Scale, 12.f, Muted);
    }
    else if (C->bMainMenu)
    {
        const auto& Map = wildlight::map_catalog()[C->MapChoice];
        const TCHAR* AI[] = {TEXT("The Builder"), TEXT("The Conqueror"), TEXT("The Arcanist")};
        const TCHAR* Difficulties[] = {TEXT("Easy"), TEXT("Normal"), TEXT("Hard")};
        Add(TEXT("Begin a skirmish"), TEXT("start"));
        Add(TEXT("Learn the light  /  Guided tutorial"), TEXT("tutorial"));
        Add(TEXT("Map: ") + Name(Map.name), TEXT("mapnext"), Name(Map.description));
        Add(FString::Printf(TEXT("Rival: %s    /    %s"), AI[C->PersonalityChoice], Difficulties[C->DifficultyChoice]), TEXT("ainext"), TEXT("Click to change personality. Use the difficulty button below."));
        Add(FString::Printf(TEXT("Difficulty: %s"), Difficulties[C->DifficultyChoice]), TEXT("difficultynext"));
        Button(TEXT("Load save"), TEXT("load"), TEXT("Restore your quicksave (F9)."), BX, BY, BW * .48f, 40.f * Scale);
        Button(TEXT("Settings"), TEXT("settings"), FString(), BX + BW * .52f, BY, BW * .48f, 40.f * Scale);
        BY += 48.f * Scale;
        Label(TEXT("Single-player strategy  |  Development build"), BX, BY + 7.f * Scale, 12.f, Muted);
    }
    else if (C->Arena && C->Arena->Simulation.state().outcome != wildlight::MatchOutcome::Playing)
    {
        const auto& S = C->Arena->Simulation.state();
        Label(S.outcome == wildlight::MatchOutcome::PlayerVictory ? TEXT("Your light endures.") : TEXT("A kingdom falls. Its story continues."), BX, BY, 20, Ink);
        BY += 44.f * Scale;
        Label(FString::Printf(TEXT("Time %d:%02d   Recruited %u   Units lost %u"), static_cast<int32>(S.tick / 600), static_cast<int32>((S.tick / 10) % 60), S.factions[0].units_recruited, S.factions[0].units_lost), BX, BY, 14, Muted);
        BY += 50.f * Scale;
        Add(TEXT("Play again"), TEXT("restart"));
        Add(TEXT("Return to the menu"), TEXT("menu"));
    }
    else
    {
        Label(TEXT("The kingdom is paused"), BX, BY - 8.f * Scale, 20, Ink);
        BY += 34.f * Scale;
        Add(TEXT("Continue"), TEXT("resume"));
        Add(TEXT("Save  /  F5"), TEXT("save"));
        Add(TEXT("Load  /  F9"), TEXT("load"));
        Add(TEXT("Settings"), TEXT("settings"));
        Add(TEXT("Restart this match"), TEXT("restart"));
        Add(TEXT("Return to the menu"), TEXT("menu"));
    }
}
void AWildlightHUD::DrawMinimap(float X, float Y, float Size)
{
    const auto* C = Cast<AWildlightPlayerController>(PlayerOwner);
    const auto& S = C->Arena->Simulation.state();
    Panel(X - 8.f * Scale, Y - 8.f * Scale, Size + 16.f * Scale, Size + 16.f * Scale, PanelColor);
    Panels.Add(FBox2D(FVector2D(X - 8.f * Scale, Y - 8.f * Scale), FVector2D(X + Size + 8.f * Scale, Y + Size + 8.f * Scale)));
    const float H = Size * S.height / S.width, DX = Size / S.width, DY = H / S.height;
    for (int32 TY = 0; TY < S.height; ++TY) for (int32 TX = 0; TX < S.width; ++TX)
    {
        const int32 Index = TY * S.width + TX;
        const bool bExplored = S.explored[0][Index] != 0;
        FLinearColor Color = !bExplored ? FLinearColor(.03f, .05f, .04f) : S.terrain[Index] == wildlight::Terrain::Water ? FLinearColor(.17f, .39f, .44f) : S.terrain[Index] == wildlight::Terrain::Rock ? FLinearColor(.38f, .39f, .32f) : FLinearColor(.27f, .39f, .25f);
        if (bExplored && !S.visible[0][Index]) Color *= .65f;
        Panel(X + TX * DX, Y + TY * DY, DX + .4f, DY + .4f, Color);
    }
    auto Dot = [&](wildlight::Vec2 P, wildlight::Owner Owner, float R)
    {
        if (Owner != wildlight::Owner::Player && !C->Arena->Simulation.is_visible(wildlight::Owner::Player, P)) return;
        Panel(X + P.x / S.width * Size - R, Y + P.y / S.height * H - R, R * 2, R * 2, Owner == wildlight::Owner::Player ? FLinearColor(.43f, .91f, .84f) : FLinearColor(.98f, .39f, .26f));
    };
    for (const auto& B : S.buildings) Dot(B.position, B.owner, 2.f * Scale);
    for (const auto& U : S.units) Dot(U.position, U.owner, 1.f * Scale);
    Label(TEXT("LIGHT ATLAS  /  click to focus"), X, Y + H + 12.f * Scale, 11.f, Gold);
    Buttons.Add({FBox2D(FVector2D(X, Y), FVector2D(X + Size, Y + H)), FString::Printf(TEXT("minimap:0:0:%d:%d"), S.width, S.height), FString(), true});
}
void AWildlightHUD::DrawMatch()
{
    auto* C = Cast<AWildlightPlayerController>(PlayerOwner);
    if (!C->Arena) return;
    const float MatchLayoutHeight = 620.f;
    const float MatchLayoutWidth = 960.f;
    Scale = FMath::Min(Scale, Canvas->ClipY / MatchLayoutHeight);
    Scale = FMath::Min(Scale, Canvas->ClipX / MatchLayoutWidth);
    const auto& S = C->Arena->Simulation.state();
    const auto& F = S.factions[0];
    Panel(0, 0, Canvas->ClipX, 62.f * Scale, PanelColor);
    Panel(0, 60.f * Scale, Canvas->ClipX, 2.f * Scale, Gold);
    Panels.Add(FBox2D(FVector2D(0, 0), FVector2D(Canvas->ClipX, 62.f * Scale)));
    const TCHAR* Labels[] = {TEXT("WOOD"), TEXT("STONE"), TEXT("FOOD"), TEXT("GOLD"), TEXT("LIGHT")};
    const double Values[] = {F.resources.wood, F.resources.stone, F.resources.food, F.resources.gold, F.resources.mana};
    const float Step = FMath::Min(132.f * Scale, (Canvas->ClipX - 240.f * Scale) / 6.f);
    for (int32 I = 0; I < 5; ++I)
    {
        Label(Labels[I], 18.f * Scale + I * Step, 9.f * Scale, 10.f, Muted);
        Label(FString::Printf(TEXT("%.0f"), Values[I]), 18.f * Scale + I * Step, 28.f * Scale, 19.f, Ink);
    }
    Label(FString::Printf(TEXT("PEOPLE  %d / %d"), F.population, F.population_limit), 18.f * Scale + 5 * Step, 12.f * Scale, 12.f, Gold);
    Label(FString::Printf(TEXT("Morale %.0f%%"), F.morale), 18.f * Scale + 5 * Step, 33.f * Scale, 11.f, Ink);
    Button(TEXT("Pause"), TEXT("pause"), TEXT("Esc pauses. F5 saves. F9 loads."), Canvas->ClipX - 118.f * Scale, 12.f * Scale, 100.f * Scale, 38.f * Scale);
    const float MapSize = 216.f * Scale;
    const float BottomY = Canvas->ClipY - 252.f * Scale;
    const float X = 16.f * Scale, W = Canvas->ClipX - MapSize - 58.f * Scale;
    const float GrowthTop = BottomY - 118.f * Scale;
    Panel(X, GrowthTop, W, 106.f * Scale, PanelColor);
    Panels.Add(FBox2D(FVector2D(X, GrowthTop), FVector2D(X + W, GrowthTop + 106.f * Scale)));
    GrowthControlsPosition = FVector2D(X + 12.f * Scale, GrowthTop + 8.f * Scale);
    GrowthControlsSize = FVector2D(W - 24.f * Scale, 90.f * Scale);
    EnsureGrowthControls();
    Panel(X, BottomY, W, 236.f * Scale, PanelColor);
    Panels.Add(FBox2D(FVector2D(X, BottomY), FVector2D(X + W, Canvas->ClipY)));
    DrawMinimap(Canvas->ClipX - MapSize - 20.f * Scale, BottomY + 14.f * Scale, MapSize);
    const wildlight::Building* B = C->Arena->Simulation.building(C->Arena->SelectedBuildingId());
    const wildlight::Unit* U = nullptr;
    for (uint32 Id : C->Arena->Selection) { if (!U) U = C->Arena->Simulation.unit(Id); }
    FString SelectionLabel = TEXT("Your kingdom");
    if (B) SelectionLabel = Name(wildlight::definition(B->kind).name) + FString::Printf(TEXT("  /  Level %d  /  %.0f%% built%s"), static_cast<int32>(B->level), B->progress * 100, B->connected ? TEXT("  /  Connected") : TEXT(""));
    else if (U) SelectionLabel = Name(wildlight::definition(U->kind).name) + FString::Printf(TEXT("  /  %d selected"), C->Arena->Selection.Num());
    Label(SelectionLabel.Left(52), X + 12.f * Scale, BottomY + 12.f * Scale, 14.f, Gold);
    const float ActionX = X + W - 285.f * Scale;
    if (B)
    {
        Button(TEXT("Repair"), TEXT("repair"), TEXT("Select workers or engineers as well as this building."), ActionX, BottomY + 8.f * Scale, 82.f * Scale, 28.f * Scale);
        const FString UpgradeTip = TEXT("Improve a completed building (maximum level 3).\n") + CostText(wildlight::building_upgrade_cost(*B));
        Button(TEXT("Upgrade"), TEXT("upgrade"), UpgradeTip, ActionX + 90.f * Scale, BottomY + 8.f * Scale, 82.f * Scale, 28.f * Scale);
        Button(TEXT("Dismantle"), TEXT("demolish"), TEXT("Dismantle the selected building and recover part of its cost."), ActionX + 180.f * Scale, BottomY + 8.f * Scale, 94.f * Scale, 28.f * Scale);
    }
    else if (U)
    {
        Button(TEXT("Stop"), TEXT("stop"), TEXT("Clear current unit orders."), ActionX, BottomY + 8.f * Scale, 82.f * Scale, 28.f * Scale);
        Button(TEXT("Patrol"), TEXT("patrol"), TEXT("Choose a patrol destination on the battlefield."), ActionX + 90.f * Scale, BottomY + 8.f * Scale, 82.f * Scale, 28.f * Scale);
        Button(TEXT("Stance"), TEXT("stance"), TEXT("Cycle defensive, aggressive and hold-position stances."), ActionX + 180.f * Scale, BottomY + 8.f * Scale, 94.f * Scale, 28.f * Scale);
    }
    const TCHAR* Tabs[] = {TEXT("Build"), TEXT("Recruit"), TEXT("Magic"), TEXT("Knowledge")};
    for (int32 I = 0; I < 4; ++I) Button(Tabs[I], FString::Printf(TEXT("tab:%d"), I), FString(), X + (12.f + I * 104.f) * Scale, BottomY + 44.f * Scale, 96.f * Scale, 28.f * Scale);
    if (ActionTab == 0)
    {
        Button(TEXT("<"), TEXT("page:prev"), TEXT("Previous buildings"), X + W - 95.f * Scale, BottomY + 44.f * Scale, 32.f * Scale, 28.f * Scale);
        Button(TEXT(">"), TEXT("page:next"), TEXT("More buildings"), X + W - 52.f * Scale, BottomY + 44.f * Scale, 32.f * Scale, 28.f * Scale);
    }
    const float BW = (W - 48.f * Scale) / 3.f;
    auto GridButton = [&](int32 Index, const FString& Text, const FString& Action, const FString& Tooltip, bool bEnabled)
    {
        const float BX = X + 12.f * Scale + (Index % 3) * (BW + 12.f * Scale);
        const float BY = BottomY + (84.f + (Index / 3) * 46.f) * Scale;
        Button(Text.Left(27), Action, Tooltip, BX, BY, BW, 38.f * Scale, bEnabled);
    };
    if (ActionTab == 0)
    {
        for (int32 I = CatalogPage * 9; I < FMath::Min(CatalogPage * 9 + 9, 19); ++I)
        {
            const auto& D = wildlight::building_catalog()[I];
            const FString Tip = Name(D.name) + TEXT("\n") + CostText(D.cost) + FString::Printf(TEXT("\nFootprint %dx%d. R rotates. Right-click cancels.%s"), D.width, D.height, D.requires_light ? TEXT(" Requires light.") : TEXT(""));
            GridButton(I % 9, Name(D.name), FString::Printf(TEXT("build:%d"), I), Tip, F.resources.can_afford(D.cost));
        }
    }
    else if (ActionTab == 1)
    {
        int32 Index = 0;
        for (int32 I = 0; I < 12; ++I)
        {
            const auto& D = wildlight::unit_catalog()[I];
            if (!B || D.recruited_at != B->kind) continue;
            GridButton(Index++, Name(D.name), FString::Printf(TEXT("train:%d"), I), Name(D.name) + TEXT("\n") + CostText(D.cost) + FString::Printf(TEXT("\nTraining %.0fs. Population %d."), D.train_seconds, D.population), B->progress >= 1 && F.resources.can_afford(D.cost));
        }
        if (!B) Label(TEXT("Select a recruiting building to view its units."), X + 15.f * Scale, BottomY + 105.f * Scale, 14.f, Ink);
        else if (!B->train_queue.empty()) Label(FString::Printf(TEXT("Training queue: %d   Next unit in %.0fs"), static_cast<int32>(B->train_queue.size()), B->train_timer), X + 15.f * Scale, BottomY + 210.f * Scale, 12.f, Gold);
    }
    else if (ActionTab == 2)
    {
        bool bWizard = false;
        for (uint32 Id : C->Arena->Selection) if (const auto* Unit = C->Arena->Simulation.unit(Id)) bWizard |= Unit->kind == wildlight::UnitKind::Wizard;
        for (int32 I = 0; I < 9; ++I)
        {
            const auto& D = wildlight::spell_catalog()[I];
            GridButton(I, Name(D.name), FString::Printf(TEXT("spell:%d"), I), Name(D.name) + FString::Printf(TEXT("\nLight %.0f. Duration %.0fs. Cooldown %.0fs. Range %.0f."), D.mana, D.duration, D.cooldown, D.cast_range), bWizard && F.resources.mana >= D.mana);
        }
    }
    else
    {
        const TCHAR* Tech[] = {TEXT("Forestry"), TEXT("Agriculture"), TEXT("Masonry"), TEXT("Steel"), TEXT("Arcana")};
        for (int32 I = 0; I < 5; ++I) GridButton(I, FString::Printf(TEXT("%s  %d/2"), Tech[I], F.technologies[I]), FString::Printf(TEXT("research:%d"), I), TEXT("Research needs the appropriate completed mill, forge or connected mage tower.\nLevel 1: 45 wood, 25 stone, 50 gold. Level 2 doubles the cost."), F.technologies[I] < 2 && F.research == wildlight::Technology::Count);
        GridButton(5, TEXT("Trade 50 wood"), TEXT("trade"), TEXT("A completed market sells 50 wood for gold."), F.resources.wood >= 50);
        if (F.research != wildlight::Technology::Count) Label(FString::Printf(TEXT("Researching %s  /  %.0fs remaining"), Tech[static_cast<int32>(F.research)], F.research_timer), X + 15.f * Scale, BottomY + 200.f * Scale, 12.f, Gold);
    }
    if (C->bBuildPreview || C->bSpellPreview || C->bPatrolPreview)
    {
        const FString Hint = C->bBuildPreview ? TEXT("Choose a building site  |  R rotates  |  Right-click cancels") : C->bSpellPreview ? TEXT("Choose a spell target  |  Right-click cancels") : TEXT("Choose a patrol point  |  Right-click cancels");
        Label(Hint, 24.f * Scale, GrowthTop - 32.f * Scale, 16.f, Gold);
    }
    else if (C->Arena->bTutorial)
    {
        const FString Hint = C->TutorialText();
        const float GuideW = FMath::Min(860.f * Scale, Canvas->ClipX - 36);
        TArray<FString> Words;
        Hint.ParseIntoArray(Words, TEXT(" "));
        TArray<FString> Lines;
        FString Current;
        const int32 MaxChars = FMath::Max(36, static_cast<int32>((GuideW - 24.f * Scale) / (7.f * Scale)));
        for (const FString& Word : Words)
        {
            if (Current.Len() + Word.Len() + 1 > MaxChars) { Lines.Add(Current); Current.Empty(); }
            if (!Current.IsEmpty()) Current += TEXT(" ");
            Current += Word;
        }
        if (!Current.IsEmpty()) Lines.Add(Current);
        Panel(18.f * Scale, 126.f * Scale, GuideW, (40.f + 19.f * Lines.Num()) * Scale, PanelColor);
        Label(TEXT("THE FIRST THREAD"), 30.f * Scale, 135.f * Scale, 11.f, Gold);
        for (int32 I = 0; I < Lines.Num(); ++I) Label(Lines[I], 30.f * Scale, (155.f + I * 19.f) * Scale, 13.f, Ink);
    }
    if (C->IsDragging())
    {
        const auto A = C->DragOrigin(), BPoint = C->MousePosition();
        const FVector2D Min(FMath::Min(A.X, BPoint.X), FMath::Min(A.Y, BPoint.Y));
        const FVector2D Max(FMath::Max(A.X, BPoint.X), FMath::Max(A.Y, BPoint.Y));
        DrawRect(FLinearColor(.45f, .83f, .73f, .10f), Min.X, Min.Y, Max.X - Min.X, Max.Y - Min.Y);
        DrawLine(Min.X, Min.Y, Max.X, Min.Y, Gold, 1.f);
        DrawLine(Max.X, Min.Y, Max.X, Max.Y, Gold, 1.f);
        DrawLine(Max.X, Max.Y, Min.X, Max.Y, Gold, 1.f);
        DrawLine(Min.X, Max.Y, Min.X, Min.Y, Gold, 1.f);
    }
    for (uint32 Id : C->Arena->Selection)
    {
        FVector Location; float Fraction = 0.f;
        if (const auto* Unit = C->Arena->Simulation.unit(Id)) { Location = C->Arena->ToWorld(Unit->position, 240.f); Fraction = Unit->health / wildlight::definition(Unit->kind).health; }
        else if (const auto* Building = C->Arena->Simulation.building(Id)) { Location = C->Arena->BuildingWorldPosition(*Building, 240.f); Fraction = Building->health / (wildlight::definition(Building->kind).health * (1.f + .35f * (Building->level - 1))); }
        else continue;
        FVector2D Screen;
        if (C->ProjectWorldLocationToScreen(Location, Screen))
        {
            Panel(Screen.X - 22, Screen.Y - 4, 44, 5, FLinearColor(.02f, .05f, .03f));
            Panel(Screen.X - 22, Screen.Y - 4, 44 * FMath::Clamp(Fraction, 0.f, 1.f), 5, FLinearColor(.49f, .83f, .46f));
        }
    }
}
