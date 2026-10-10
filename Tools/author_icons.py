#!/usr/bin/env python3
"""Original outlined game glyph studies; explicit SVG paths, no stock images.

Requires CairoSVG for raster exports. Editable SVG masters remain usable without
the renderer. The sheet is a content preview, not a gameplay screenshot.
"""
import hashlib
import json
from pathlib import Path
import cairosvg
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Art/UI"

# Each entry uses a distinct equipment/building silhouette at a 64x64 design grid.
GLYPHS = {
    "Resource_Wood": "M19 48V26M45 48V26M15 26Q32 9 49 26L45 43Q32 51 19 43M19 26Q32 34 45 26M26 22Q32 15 38 22Q32 29 26 22Z",
    "Resource_Stone": "M10 47L15 29L30 14L46 20L54 42L41 51Z M15 29L32 35L46 20M32 35L41 51M32 35L30 14",
    "Resource_Food": "M32 52V14M32 25Q17 23 20 14Q31 14 32 25M32 34Q16 34 18 25Q29 25 32 34M32 43Q15 44 18 35Q29 35 32 43M32 29Q47 27 45 18Q34 18 32 29M32 38Q48 36 46 27Q35 27 32 38",
    "Resource_Gold": "M14 41Q14 34 32 34Q50 34 50 41V47Q50 54 32 54Q14 54 14 47Z M14 41Q32 49 50 41M21 28Q21 18 32 18Q43 18 43 28Q43 38 32 38Q21 38 21 28Z M32 22V33",
    "Resource_Mana": "M32 8L46 25L40 47L32 55L24 47L18 25Z M18 25H46M32 8V55M18 25L32 36L46 25",
    "Building_Castle": "M10 52V24H20V32H25V18H39V32H44V24H54V52Z M10 24V13H15V18H20V13M44 24V13H49V18H54V13M27 52V43Q32 34 37 43V52M29 18V10H35V18",
    "Building_House": "M12 29L32 11L52 29M17 27V52H47V27M27 52V36H37V52M22 22V13H28",
    "Building_Farm": "M10 32L24 20L38 32V48H10Z M20 48V37H28V48M43 53V15M43 29L36 21M43 24L51 16M43 38L52 30M8 54H56",
    "Building_Mill": "M24 53L26 29H38L40 53Z M32 29V13M32 22L18 8M32 22L46 8M32 22L47 37M32 22L17 37M29 53V43H35V53",
    "Building_Depot": "M10 23L32 12L54 23M14 23V51H50V23M14 35H50M23 51V39M41 51V39M14 51L32 37L50 51",
    "Building_LumberCamp": "M12 52V38L26 24L40 38V52Z M37 15L47 12L55 23L45 32L37 15Z M41 24L25 49M18 41H33",
    "Building_Quarry": "M12 49L18 31L34 25L50 35L54 49Z M18 31L28 42L50 35M28 42V49M18 11Q36 9 48 26M32 15L25 32",
    "Building_GoldMine": "M9 51L18 23L31 13L48 27L55 51Z M22 51V38Q32 23 42 38V51M27 39L32 34L37 39L32 44Z",
    "Building_Barracks": "M12 51V26H52V51M9 26L32 12L55 26M24 42L40 28M25 29L39 43M21 42L27 48M37 27L43 33",
    "Building_ArcherRange": "M20 12Q48 32 20 52V12M20 12L34 32L20 52M12 32H52M44 25L52 32L44 39",
    "Building_Forge": "M10 31H51L44 39H25L20 45V51H40M26 31V22H42L46 31M15 22L27 10M20 11L29 20M46 17L51 12",
    "Building_Market": "M12 24L18 13H46L52 24V29Q47 37 42 29Q37 37 32 29Q27 37 22 29Q17 37 12 29Z M16 35V52H48V35M22 44H42M22 18V25M32 18V25M42 18V25",
    "Building_Wall": "M9 52V22H17V15H25V22H39V15H47V22H55V52Z M9 37H55M25 22V37M39 37V52",
    "Building_Gate": "M10 52V17H20V52M44 52V17H54V52M20 31Q32 14 44 31M24 52V32H40V52M28 34V51M36 34V51M24 40H40",
    "Building_Watchtower": "M22 52L20 27H44L42 52Z M17 27V14H24V20H29V14H35V20H40V14H47V27Z M28 37H36V43H28Z",
    "Building_Beacon": "M18 52H46M23 48L27 32H37L41 48M32 9L42 22L32 35L22 22Z M9 22H15M49 22H55M16 6L21 11M48 6L43 11",
    "Building_Sanctuary": "M11 48H53M16 48V23M48 48V23M9 23Q32 6 55 23Z M25 46L32 26L39 46Z M24 48V52H40V48",
    "Building_MageTower": "M22 53L24 28H40L42 53Z M15 28L32 8L49 28Z M29 35H35V43H29Z M17 9L12 14M49 9L54 14",
    "Building_SiegeWorkshop": "M10 30L31 14L54 30M14 29V52H49V29M20 40L42 31M24 52L40 31M18 49Q18 44 23 44Q28 44 28 49Q28 54 23 54Q18 54 18 49M39 49Q39 44 44 44Q49 44 49 49Q49 54 44 54Q39 54 39 49",
    "Unit_Worker": "M32 12A6 6 0 1 0 32 24A6 6 0 1 0 32 12M23 36L32 28L41 36M32 28V45L23 54M32 45L41 54M12 21Q21 11 30 13M20 15L12 41",
    "Unit_Militia": "M18 22A7 7 0 1 0 18 36A7 7 0 1 0 18 22M12 41V53H25V41M39 10V46M32 40H46M35 47L39 53L43 47",
    "Unit_Spearman": "M23 21A6 6 0 1 0 23 33A6 6 0 1 0 23 21M17 37V53H30V37M43 10L48 22H38Z M43 22V54M30 40H43",
    "Unit_Swordsman": "M17 51L42 15L51 10L48 21L23 54Z M12 42L28 54M20 36L32 44M12 18L24 10L32 20L25 31L12 28Z",
    "Unit_Archer": "M23 12Q52 32 23 52V12M23 12L39 32L23 52M10 32H54M48 27L54 32L48 37",
    "Unit_Cavalry": "M13 51L16 33L25 29L30 18L41 10L49 27L42 34L40 51M19 51L24 38H37L35 51M35 18L42 22M23 29L31 34",
    "Unit_Engineer": "M13 18L24 9L36 21L25 32Z M26 26L48 51M14 51L47 18M41 10L39 18L46 25L54 23M9 51L14 56",
    "Unit_ArcaneGuard": "M12 19L32 10L52 19V35Q50 49 32 56Q14 49 12 35Z M32 20L41 33L32 45L23 33Z M32 20V45",
    "Unit_ForestAlly": "M32 12V53M32 27Q16 30 12 17Q26 12 32 27M32 40Q47 44 52 29Q37 25 32 40M32 18L40 10M20 53L32 45L44 53",
    "Unit_Ram": "M10 32H52V43H10Z M15 25H48M47 32L55 37L47 43M14 48A5 5 0 1 0 24 48A5 5 0 1 0 14 48M39 48A5 5 0 1 0 49 48A5 5 0 1 0 39 48M19 32V25M43 32V25",
    "Unit_Catapult": "M15 47H49M22 46L29 26L41 45M29 29L44 11M37 11H51V19H37Z M13 49A5 5 0 1 0 23 49A5 5 0 1 0 13 49M41 49A5 5 0 1 0 51 49A5 5 0 1 0 41 49",
    "Unit_Wizard": "M14 30L30 10L39 30Z M20 32L15 53H43L34 32M49 23V53M49 9L55 17L49 25L43 17Z",
    "Spell_RootBarrier": "M10 52H54M15 49Q25 40 18 29Q14 20 22 12M24 49Q34 35 30 14M39 49Q36 35 48 21M20 32L10 25M31 27L42 17M43 32L53 31",
    "Spell_Heal": "M27 12H37V27H52V37H37V52H27V37H12V27H27Z",
    "Spell_HarvestBloom": "M32 35V54M32 35Q21 20 14 29Q11 40 32 35M32 35Q46 17 52 28Q54 41 32 35M32 33Q17 13 28 10Q40 10 32 33",
    "Spell_LightBridge": "M9 45Q32 15 55 45M13 40V51M22 30V47M32 25V45M42 30V47M51 40V51M9 55Q20 49 31 55Q42 49 55 55M24 9H40",
    "Spell_Amplify": "M32 16L44 32L32 48L20 32Z M10 22L5 32L10 42M54 22L59 32L54 42M20 9L32 4L44 9M20 55L32 60L44 55",
    "Spell_Shield": "M12 19L32 9L52 19V35Q48 50 32 56Q16 50 12 35Z M32 18V45M24 32H40",
    "Spell_Veil": "M9 23Q23 14 35 23Q48 31 55 23M9 33Q23 24 35 33Q48 41 55 33M9 43Q23 34 35 43Q48 51 55 43M32 10V15",
    "Spell_Disrupt": "M11 18L25 27L18 38L31 47M53 18L40 27L47 38L33 47M28 9L35 23L28 32L36 45L31 56",
    "Spell_Illusion": "M8 32Q32 10 56 32Q32 54 8 32Z M26 32A6 6 0 1 0 38 32A6 6 0 1 0 26 32M11 15L15 11M49 53L53 49M49 11L53 15",
    "Technology_Forestry": "M32 9L13 35H22L14 46H28V55H36V46H50L42 35H51Z",
    "Technology_Agriculture": "M16 51Q16 27 37 17M22 34Q17 23 25 20M32 24Q42 22 45 13M23 44Q33 41 34 34M12 52H53",
    "Technology_Masonry": "M10 20H54V51H10Z M10 35H54M26 20V35M40 35V51M12 13H52",
    "Technology_Steel": "M13 45L42 16L50 10L49 20L20 50Z M12 38L27 53M30 18L18 10M12 16L24 28",
    "Technology_Arcana": "M32 8L39 23L55 26L43 38L45 55L32 47L19 55L21 38L9 26L25 23Z M28 31L32 25L36 31L32 38Z",
}


def main():
    svg_dir, png_dir = OUTPUT / "SVG", OUTPUT / "PNG"
    svg_dir.mkdir(parents=True, exist_ok=True)
    png_dir.mkdir(parents=True, exist_ok=True)
    entries = []
    sheet = Image.new("RGB", (880, 7 * 132 + 50), "#1f322b")
    draw = ImageDraw.Draw(sheet)
    draw.text((18, 15), "WILDLIGHT / ORIGINAL INTERFACE GLYPH STUDIES", fill="#e7c477")
    for index, (identifier, path) in enumerate(GLYPHS.items()):
        svg = f'<svg xmlns="http://www.w3.org/2000/svg" width="128" height="128" viewBox="0 0 64 64"><title>{identifier}</title><path d="{path}" stroke="#E6C987" fill="none" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/></svg>\n'
        master = svg_dir / (identifier + ".svg")
        master.write_text(svg, encoding="utf-8")
        png = png_dir / (identifier + ".png")
        cairosvg.svg2png(bytestring=svg.encode(), write_to=str(png))
        tile = Image.open(png).convert("RGBA")
        tile.thumbnail((72, 72))
        x, y = (index % 8) * 110, 50 + (index // 8) * 132
        sheet.paste(tile, (x + 19, y), tile)
        words = identifier.split("_", 1)
        draw.text((x + 5, y + 79), words[0], fill="#8fa891")
        draw.text((x + 5, y + 96), words[1][:17], fill="#e8dfc7")
        entries.append({"id": identifier, "asset_name": "T_" + identifier,
                        "svg": "UI/SVG/" + master.name, "png": "UI/PNG/" + png.name,
                        "sha256": hashlib.sha256(png.read_bytes()).hexdigest()})
    sheet.save(OUTPUT / "Glyph_Studies.png")
    (OUTPUT / "manifest.json").write_text(json.dumps({"format_version": 1,
        "authorship": "Original explicit vector paths in Tools/author_icons.py; no external images or image-generation model",
        "status": "Prototype interface glyphs; in-engine layout, contrast and recognition review pending",
        "assets": entries}, indent=2) + "\n", encoding="utf-8")
    print(f"Authored {len(entries)} original editable SVG/PNG glyph studies")


if __name__ == "__main__":
    main()
