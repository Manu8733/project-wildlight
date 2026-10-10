"""Static adapter contracts only; these do not compile or exercise Unreal."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORLD = ROOT / "Source/Wildlight/Private/WildlightWorld.cpp"
HUD = ROOT / "Source/Wildlight/Private/WildlightHUD.cpp"


def function_body(source: str, name: str) -> str:
    """Extract a named C++ definition for narrow source wiring checks."""
    start = source.find(name + "(")
    if start < 0:
        raise AssertionError(f"Missing adapter function: {name}")
    opening = source.find("{", start)
    depth = 1
    cursor = opening + 1
    while depth and cursor < len(source):
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    if depth:
        raise AssertionError(f"Unclosed adapter function: {name}")
    return source[opening + 1 : cursor - 1]


class GrowthAdapterContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.world = WORLD.read_text(encoding="utf-8")
        cls.hud = HUD.read_text(encoding="utf-8")

    def test_policy_is_submitted_through_owner_validated_command(self):
        policy = function_body(self.world, "AWildlightWorld::SetAutomaticGrowthPolicy")
        self.assertIn("Command.type = wildlight::CommandType::SetAutomaticGrowth;", policy)
        self.assertIn("Command.owner = wildlight::Owner::Player;", policy)
        self.assertIn("Command.automatic_growth = Policy;", policy)
        self.assertIn("return Submit(Command);", policy)
        self.assertTrue("->SetAutomaticGrowthPolicy(" in self.hud, "HUD must submit policy through World")

    def test_selected_building_reads_core_status_and_level(self):
        self.assertTrue("automatic_growth_status(B->id)" in self.hud, "HUD must read Core eligibility")
        self.assertTrue("B->level" in self.hud, "HUD must show the authoritative building level")

    def test_level_variants_refresh_cached_actors(self):
        for kind in ("House", "Farm"):
            for level in (1, 2, 3):
                self.assertTrue(f'SM_{kind}_L{level}' in self.world, f"Missing tier asset lookup: {kind} L{level}")
        refresh = function_body(self.world, "AWildlightWorld::RefreshPresentation")
        self.assertIn("PresentedBuildingLevels.Find(B.id)", refresh)
        self.assertIn("PresentedBuildingLevels.Add(B.id, B.level)", refresh)

    def test_load_initializes_saved_tick_event_dedup_before_refresh(self):
        load = function_body(self.world, "AWildlightWorld::LoadMatch")
        self.assertTrue("InitializeEventCursorFromSave();" in load, "Load must initialize saved event dedup")
        self.assertTrue("RefreshPresentation();" in load, "Load must refresh authoritative presentation")
        self.assertLess(load.index("InitializeEventCursorFromSave();"), load.index("RefreshPresentation();"))
        initialize = function_body(self.world, "AWildlightWorld::InitializeEventCursorFromSave")
        self.assertIn("LastEventTick = Simulation.state().tick;", initialize)
        self.assertIn("Simulation.state().events", initialize)
        self.assertIn("Event.tick == LastEventTick", initialize)
        self.assertIn("EventsAtLastTick.Add(", initialize)

    def test_upgrade_cost_text_preserves_fractional_prices(self):
        cost_text = function_body(self.hud, "CostText")
        self.assertTrue('Wood %.2f   Stone %.2f' in cost_text, "Upgrade wood/stone prices need two decimal places")
        self.assertNotIn("%.0f", cost_text)
        match = function_body(self.hud, "AWildlightHUD::DrawMatch")
        self.assertIn("CostText(wildlight::building_upgrade_cost(*B))", match)

    def test_match_scale_reserves_space_for_guidance_before_panels(self):
        match = function_body(self.hud, "AWildlightHUD::DrawMatch")
        declaration = re.search(r"const float MatchLayoutHeight = ([0-9.]+)f;", match)
        self.assertIsNotNone(declaration, "Match needs an explicit vertical fit budget for its new growth band")
        self.assertGreaterEqual(float(declaration.group(1)), 370 + 126 + 40 + 3 * 19 + 12,
                                "Budget must fit the bottom stack plus a three-line guide and gap")
        fit = "Scale = FMath::Min(Scale, Canvas->ClipY / MatchLayoutHeight);"
        self.assertTrue(fit in match, "Match scale must respect the viewport height")
        self.assertLess(match.index(fit), match.index("Panel("))

    def test_action_tooltips_use_slate_above_growth_with_cleanup(self):
        tooltip = function_body(self.hud, "AWildlightHUD::EnsureActionTooltip")
        growth = function_body(self.hud, "AWildlightHUD::EnsureGrowthControls")
        tooltip_layer = re.search(r"AddViewportWidgetContent\(ActionTooltip.ToSharedRef\(\), (\d+)\)", tooltip)
        growth_layer = re.search(r"AddViewportWidgetContent\(GrowthControls.ToSharedRef\(\), (\d+)\)", growth)
        self.assertIsNotNone(tooltip_layer, "Action tooltip must be registered with Slate")
        self.assertIsNotNone(growth_layer, "Growth controls must have an explicit layer")
        self.assertGreater(int(tooltip_layer.group(1)), int(growth_layer.group(1)), "Tooltips must draw above growth inputs")
        draw = function_body(self.hud, "AWildlightHUD::DrawHUD")
        self.assertIn("ActionTooltipText = FText::FromString(B.Tooltip);", draw)
        end = function_body(self.hud, "AWildlightHUD::EndPlay")
        self.assertIn("RemoveViewportWidgetContent(ActionTooltip.ToSharedRef())", end)


if __name__ == "__main__":
    unittest.main()
