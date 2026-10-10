#include "wildlight/Simulation.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_set>
#include <utility>

namespace wildlight {
namespace {
constexpr double Pi = 3.14159265358979323846;
constexpr std::size_t MaximumEntities = 4096;
constexpr std::size_t MaximumSaveBytes = 8 * 1024 * 1024;
int side(Owner owner) { return owner == Owner::Enemy ? 1 : 0; }
double distance(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }
bool finite(Vec2 p) { return std::isfinite(p.x) && std::isfinite(p.y); }
Vec2 centre(Vec2 p) { return {std::floor(p.x) + 0.5, std::floor(p.y) + 0.5}; }
Resources resources(double w = 0, double s = 0, double f = 0, double g = 0, double m = 0) {
    return {w, s, f, g, m};
}
void subtract(Resources &stock, const Resources &cost) {
    stock.wood -= cost.wood;
    stock.stone -= cost.stone;
    stock.food -= cost.food;
    stock.gold -= cost.gold;
    stock.mana -= cost.mana;
}
void deposit(FactionState &faction, ResourceKind resource, double amount) {
    faction.resources.add(resource,
                          std::min(amount, std::max(0.0, faction.capacity.get(resource) -
                                                             faction.resources.get(resource))));
}
bool complete(const Building &b) { return b.health > 0 && b.progress >= 1; }
bool grows_automatically(BuildingKind kind) {
    return kind == BuildingKind::House || kind == BuildingKind::Farm;
}
double farm_milestone(const Building &building) {
    return building.kind != BuildingKind::Farm || building.level >= 3 ? 0
           : building.level == 1 ? 120 : 240;
}
bool valid_growth_policy(const AutomaticGrowthPolicy &policy) {
    return std::isfinite(policy.protected_wood) && policy.protected_wood >= 0 &&
           policy.protected_wood <= 1000000 && std::isfinite(policy.protected_stone) &&
           policy.protected_stone >= 0 && policy.protected_stone <= 1000000;
}
bool storage(BuildingKind k) { return k == BuildingKind::Castle || k == BuildingKind::Depot; }
bool light_node(BuildingKind k) {
    return k == BuildingKind::Castle || k == BuildingKind::Beacon || k == BuildingKind::Sanctuary;
}
bool producer(BuildingKind k) {
    return k == BuildingKind::Farm || k == BuildingKind::LumberCamp || k == BuildingKind::Quarry ||
           k == BuildingKind::GoldMine;
}
ResourceKind output(BuildingKind k) {
    if (k == BuildingKind::Quarry)
        return ResourceKind::Stone;
    if (k == BuildingKind::GoldMine)
        return ResourceKind::Gold;
    if (k == BuildingKind::Farm)
        return ResourceKind::Food;
    return ResourceKind::Wood;
}
struct Rect {
    int x, y, w, h;
};
Rect footprint(BuildingKind kind, Vec2 p, std::uint8_t rotation) {
    const auto &d = definition(kind);
    int w = d.width, h = d.height;
    if (rotation % 2)
        std::swap(w, h);
    return {static_cast<int>(std::floor(p.x)) - w / 2, static_cast<int>(std::floor(p.y)) - h / 2, w,
            h};
}
bool contains(Rect r, Vec2 p, double margin = 0) {
    return p.x >= r.x - margin && p.y >= r.y - margin && p.x < r.x + r.w + margin &&
           p.y < r.y + r.h + margin;
}
bool overlap(Rect a, Rect b) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}
double building_distance(const Building &b, Vec2 p) {
    Rect r = footprint(b.kind, b.position, b.rotation);
    return std::hypot(std::max({static_cast<double>(r.x) - p.x, 0.0, p.x - (r.x + r.w)}),
                      std::max({static_cast<double>(r.y) - p.y, 0.0, p.y - (r.y + r.h)}));
}
double max_health(const Building &b) {
    return definition(b.kind).health * (1 + 0.35 * (b.level - 1));
}
double max_health(const Unit &u) { return definition(u.kind).health; }
bool concealed(const State &state, const Unit &target, Owner viewer) {
    for (const auto &effect : state.effects)
        if (effect.remaining > 0 && effect.kind == SpellKind::Veil &&
            effect.owner == target.owner &&
            distance(effect.position, target.position) <= effect.radius) {
            for (const auto &observer : state.units)
                if (observer.health > 0 && observer.owner == viewer &&
                    distance(observer.position, target.position) <= 2.5)
                    return false;
            for (const auto &b : state.buildings)
                if (b.owner == viewer && complete(b) && b.kind == BuildingKind::Watchtower &&
                    distance(b.position, target.position) <= 4)
                    return false;
            return true;
        }
    return false;
}
void clear_path(Unit &u) {
    u.path.clear();
    u.path_index = 0;
}
CommandResult reject(std::string reason) { return {false, std::move(reason), 0}; }
CommandResult accept(Id id = 0) { return {true, "", id}; }
bool valid_resources(const Resources &r) {
    for (auto k : {ResourceKind::Wood, ResourceKind::Stone, ResourceKind::Food, ResourceKind::Gold,
                   ResourceKind::Mana})
        if (!std::isfinite(r.get(k)) || r.get(k) < 0 || r.get(k) > 10000000)
            return false;
    return true;
}
} // namespace

const std::array<BuildingDefinition, 19> &building_catalog() {
    static const std::array<BuildingDefinition, 19> catalog{
        {{BuildingKind::Castle, "Dawnkeep", resources(300, 250, 0, 100), 4, 4, 1800, 45, 18, 1200,
          0, 15, false},
         {BuildingKind::House, "Hearth Cottage", resources(45, 10), 2, 2, 260, 8, 8, 0, 0, 0,
          false},
         {BuildingKind::Farm, "Sungrain Farm", resources(70, 10), 3, 3, 320, 12, 0, 0, 3, 0, false},
         {BuildingKind::Mill, "Windweave Mill", resources(90, 30, 0, 15), 2, 2, 360, 14, 0, 0, 0, 0,
          false},
         {BuildingKind::Depot, "Wayfarer Depot", resources(65, 25), 2, 2, 420, 10, 0, 600, 0, 0,
          false},
         {BuildingKind::LumberCamp, "Timber Lodge", resources(55, 10), 2, 2, 300, 9, 0, 0, 3, 0,
          false},
         {BuildingKind::Quarry, "Stonecutters' Yard", resources(70, 15), 2, 2, 350, 12, 0, 0, 3, 0,
          false},
         {BuildingKind::GoldMine, "Gilded Hollow", resources(80, 35), 2, 2, 350, 14, 0, 0, 3, 0,
          false},
         {BuildingKind::Barracks, "Sentinel Barracks", resources(110, 55, 0, 20), 3, 3, 650, 18, 0,
          0, 0, 0, false},
         {BuildingKind::ArcherRange, "Willow Range", resources(100, 30, 0, 25), 3, 2, 480, 16, 0, 0,
          0, 0, false},
         {BuildingKind::Forge, "Ember Forge", resources(85, 65, 0, 40), 2, 2, 580, 18, 0, 0, 0, 0,
          false},
         {BuildingKind::Market, "Lantern Market", resources(100, 40, 0, 45), 3, 2, 460, 18, 0, 0, 0,
          0, false},
         {BuildingKind::Wall, "Runestone Wall", resources(0, 12), 1, 1, 520, 4, 0, 0, 0, 0, false},
         {BuildingKind::Gate, "Oath Gate", resources(20, 35), 1, 3, 700, 9, 0, 0, 0, 0, false},
         {BuildingKind::Watchtower, "Thornwatch Tower", resources(55, 85, 0, 25), 2, 2, 850, 18, 0,
          0, 0, 0, false},
         {BuildingKind::Beacon, "Threadlight Beacon", resources(65, 50, 0, 30, 15), 1, 1, 430, 12,
          0, 0, 0, 13, false},
         {BuildingKind::Sanctuary, "Grove Sanctuary", resources(90, 90, 0, 65, 30), 3, 3, 650, 22,
          0, 0, 0, 14, true},
         {BuildingKind::MageTower, "Starweaver Spire", resources(100, 110, 0, 90, 35), 2, 2, 580,
          24, 0, 0, 0, 0, true},
         {BuildingKind::SiegeWorkshop, "Ironroot Workshop", resources(140, 80, 0, 70), 3, 3, 700,
          24, 0, 0, 0, 0, false}}};
    return catalog;
}
const std::array<UnitDefinition, 12> &unit_catalog() {
    static const std::array<UnitDefinition, 12> catalog{
        {{UnitKind::Worker, "Hearthfolk", resources(0, 0, 25), BuildingKind::Castle, 65, 4, 0, 0.9,
          2.2, 1.2, 6, 1, false},
         {UnitKind::Militia, "Lantern Militia", resources(12, 0, 30, 8), BuildingKind::Barracks,
          100, 12, 1, 1.1, 2.4, 1.0, 7, 1, false},
         {UnitKind::Spearman, "Briar Spearman", resources(20, 0, 35, 15), BuildingKind::Barracks,
          125, 16, 2, 1.5, 2.25, 1.1, 9, 1, false},
         {UnitKind::Swordsman, "Oathblade", resources(10, 12, 40, 30), BuildingKind::Barracks, 155,
          22, 5, 1.0, 2.1, 1.0, 11, 1, false},
         {UnitKind::Archer, "Willow Archer", resources(25, 0, 35, 20), BuildingKind::ArcherRange,
          85, 15, 1, 6.5, 2.3, 1.4, 10, 1, false},
         {UnitKind::Cavalry, "Dawnrider", resources(20, 0, 65, 45), BuildingKind::Barracks, 220, 26,
          4, 1.2, 3.8, 1.2, 16, 2, false},
         {UnitKind::Engineer, "Runesmith", resources(30, 10, 40, 25), BuildingKind::SiegeWorkshop,
          95, 7, 2, 1.0, 2.1, 1.5, 12, 1, false},
         {UnitKind::ArcaneGuard, "Threadbound Guard", resources(0, 25, 40, 45, 25),
          BuildingKind::MageTower, 190, 24, 6, 1.4, 2.2, 1.2, 16, 2, false},
         {UnitKind::ForestAlly, "Grove Warden", resources(20, 0, 50, 20, 30),
          BuildingKind::Sanctuary, 210, 20, 3, 1.4, 2.0, 1.4, 15, 2, false},
         {UnitKind::Ram, "Ironroot Ram", resources(90, 30, 0, 45), BuildingKind::SiegeWorkshop, 380,
          32, 8, 1.2, 1.4, 2.2, 22, 3, true},
         {UnitKind::Catapult, "Moonstone Catapult", resources(110, 60, 0, 65),
          BuildingKind::SiegeWorkshop, 220, 42, 3, 9.5, 1.1, 3.8, 26, 3, true},
         {UnitKind::Wizard, "Wildlight Weaver", resources(0, 15, 35, 65, 35),
          BuildingKind::MageTower, 90, 18, 1, 5.8, 2.1, 1.6, 18, 2, false}}};
    return catalog;
}
const std::array<SpellDefinition, 9> &spell_catalog() {
    static const std::array<SpellDefinition, 9> catalog{
        {{SpellKind::RootBarrier, MagicSchool::Roots, "Briar Rampart", 25, 18, 26, 1.8, 13},
         {SpellKind::Heal, MagicSchool::Roots, "Kindred Renewal", 20, 2, 18, 4, 12},
         {SpellKind::HarvestBloom, MagicSchool::Roots, "Sungrain Bloom", 25, 40, 48, 6, 14},
         {SpellKind::LightBridge, MagicSchool::Light, "Threadway", 30, 30, 38, 3.7, 15},
         {SpellKind::Amplify, MagicSchool::Light, "Beacon Chorus", 25, 35, 42, 6, 14},
         {SpellKind::Shield, MagicSchool::Light, "Oathlight Aegis", 30, 22, 30, 4.5, 12},
         {SpellKind::Veil, MagicSchool::Shadows, "Mothveil", 25, 20, 28, 5, 12},
         {SpellKind::Disrupt, MagicSchool::Shadows, "Unravel", 35, 24, 35, 4.5, 14},
         {SpellKind::Illusion, MagicSchool::Shadows, "False Dawn", 30, 20, 30, 3, 12}}};
    return catalog;
}
const std::array<MapDefinition, 3> &map_catalog() {
    static const std::array<MapDefinition, 3> catalog{
        {{"The Threadwater Vale",
          "A silver river divides two woodland kingdoms. The old ford is permanent; the northern "
          "and southern crossings need Threadway.",
          64,
          48,
          {9.5, 24.5},
          {54.5, 24.5},
          0,
          0},
         {"Emberfall Pass",
          "Two rocky ridges funnel armies through narrow woodland passes. Control the ruins beyond "
          "the twin streams.",
          64,
          48,
          {9.5, 12.5},
          {54.5, 35.5},
          0,
          0},
         {"The Lantern Mere",
          "Three ancient lantern sites encircle a lake. Connect beacons at any two for 90 seconds, "
          "or destroy the rival Dawnkeep.",
          64,
          48,
          {9.5, 24.5},
          {54.5, 24.5},
          2,
          90}}};
    return catalog;
}
const BuildingDefinition &definition(BuildingKind k) {
    return building_catalog()[std::min<std::size_t>(static_cast<std::size_t>(k), 18)];
}
const UnitDefinition &definition(UnitKind k) {
    return unit_catalog()[std::min<std::size_t>(static_cast<std::size_t>(k), 11)];
}
const SpellDefinition &definition(SpellKind k) {
    return spell_catalog()[std::min<std::size_t>(static_cast<std::size_t>(k), 8)];
}
double Resources::get(ResourceKind k) const {
    switch (k) {
    case ResourceKind::Wood:
        return wood;
    case ResourceKind::Stone:
        return stone;
    case ResourceKind::Food:
        return food;
    case ResourceKind::Gold:
        return gold;
    case ResourceKind::Mana:
        return mana;
    }
    return 0;
}
void Resources::add(ResourceKind k, double a) {
    switch (k) {
    case ResourceKind::Wood:
        wood += a;
        break;
    case ResourceKind::Stone:
        stone += a;
        break;
    case ResourceKind::Food:
        food += a;
        break;
    case ResourceKind::Gold:
        gold += a;
        break;
    case ResourceKind::Mana:
        mana += a;
        break;
    }
}
bool Resources::can_afford(const Resources &c) const {
    return wood + 1e-8 >= c.wood && stone + 1e-8 >= c.stone && food + 1e-8 >= c.food &&
           gold + 1e-8 >= c.gold && mana + 1e-8 >= c.mana;
}
Resources building_upgrade_cost(const Building &building) {
    Resources cost = definition(building.kind).cost;
    const double multiplier = 0.65 * building.level;
    cost.wood *= multiplier;
    cost.stone *= multiplier;
    cost.gold *= multiplier;
    cost.food = 0;
    cost.mana = 0;
    return cost;
}

Simulation::Simulation() { new_match(); }
std::uint32_t Simulation::random() {
    auto &x = state_.rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}
const Building *Simulation::building(Id id) const {
    for (const auto &b : state_.buildings)
        if (b.id == id && b.health > 0)
            return &b;
    return nullptr;
}
const Unit *Simulation::unit(Id id) const {
    for (const auto &u : state_.units)
        if (u.id == id && u.health > 0)
            return &u;
    return nullptr;
}
const ResourceNode *Simulation::node(Id id) const {
    for (const auto &n : state_.nodes)
        if (n.id == id)
            return &n;
    return nullptr;
}
Building *Simulation::mutable_building(Id id) {
    for (auto &b : state_.buildings)
        if (b.id == id && b.health > 0)
            return &b;
    return nullptr;
}
Unit *Simulation::mutable_unit(Id id) {
    for (auto &u : state_.units)
        if (u.id == id && u.health > 0)
            return &u;
    return nullptr;
}
ResourceNode *Simulation::mutable_node(Id id) {
    for (auto &n : state_.nodes)
        if (n.id == id)
            return &n;
    return nullptr;
}
void Simulation::event(Owner owner, Id id, const std::string &text) {
    state_.events.push_back({state_.tick, owner, id, text});
    if (state_.events.size() > 80)
        state_.events.erase(state_.events.begin());
}
Id Simulation::add_building(Owner owner, BuildingKind kind, Vec2 p, bool completed) {
    Building b;
    b.id = state_.next_id++;
    b.owner = owner;
    b.kind = kind;
    b.position = centre(p);
    b.health = definition(kind).health;
    b.progress = completed ? 1 : 0;
    b.rally = {std::clamp(p.x + 3, 1.5, state_.width - 1.5),
               std::clamp(p.y + 3, 1.5, state_.height - 1.5)};
    state_.buildings.push_back(b);
    return b.id;
}
Id Simulation::add_unit(Owner owner, UnitKind kind, Vec2 p) {
    Unit u;
    u.id = state_.next_id++;
    u.owner = owner;
    u.kind = kind;
    u.position = p;
    u.destination = p;
    u.patrol_origin = p;
    u.health = definition(kind).health;
    state_.units.push_back(u);
    return u.id;
}
void Simulation::new_match(std::uint32_t index, std::uint32_t seed, AIPersonality personality,
                           Difficulty difficulty) {
    state_ = State{};
    state_.map_index = index % 3;
    state_.rng = seed ? seed : 1;
    state_.ai_personality =
        static_cast<unsigned>(personality) < 3 ? personality : AIPersonality::Builder;
    state_.difficulty = static_cast<unsigned>(difficulty) < 3 ? difficulty : Difficulty::Normal;
    const auto &map = map_catalog()[state_.map_index];
    state_.width = map.width;
    state_.height = map.height;
    state_.terrain.assign(static_cast<std::size_t>(map.width * map.height), Terrain::Grass);
    auto paint = [&](int x, int y, Terrain t) {
        if (x >= 0 && y >= 0 && x < map.width && y < map.height)
            state_.terrain[static_cast<std::size_t>(y * map.width + x)] = t;
    };
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x) {
            if (x == 0 || y == 0 || x == map.width - 1 || y == map.height - 1) {
                paint(x, y, Terrain::Rock);
                continue;
            }
            if ((x * 17 + y * 31 + static_cast<int>(state_.map_index) * 11) % 23 < 4)
                paint(x, y, Terrain::Forest);
        }
    if (state_.map_index == 0) {
        for (int y = 1; y < 47; ++y)
            for (int x = 30; x <= 33; ++x)
                paint(x, y, Terrain::Water);
        for (int y = 22; y <= 25; ++y)
            for (int x = 30; x <= 33; ++x)
                paint(x, y, Terrain::Bridge);
        state_.crossing_sites = {{32, 11.5}, {32, 36.5}};
        state_.objective_sites = {{24.5, 11.5}, {39.5, 36.5}};
    } else if (state_.map_index == 1) {
        for (int y = 5; y < 43; ++y)
            for (int x : {24, 25, 38, 39})
                if (y < 19 || y > 28)
                    paint(x, y, Terrain::Rock);
        for (int y = 19; y <= 28; ++y)
            for (int x : {24, 25, 38, 39})
                paint(x, y, Terrain::Ruin);
        for (int x = 28; x <= 35; ++x)
            for (int y : {9, 10, 37, 38})
                paint(x, y, Terrain::Water);
        state_.crossing_sites = {{32, 9.5}, {32, 37.5}};
        state_.objective_sites = {{31.5, 23.5}};
    } else {
        for (int y = 11; y < 37; ++y)
            for (int x = 23; x < 41; ++x)
                if (std::hypot((x - 31.5) / 1.0, (y - 23.5) / 1.3) < 10)
                    paint(x, y, Terrain::Water);
        for (int y = 22; y <= 25; ++y)
            for (int x = 22; x <= 41; ++x)
                paint(x, y, Terrain::Bridge);
        state_.crossing_sites = {{32, 13.5}, {32, 34.5}};
        // Crossings on this lake are shorter lateral paths; all sites touch water.
        state_.crossing_sites = {{23.5, 16.5}, {40.5, 31.5}};
        state_.objective_sites = {{31.5, 6.5}, {31.5, 23.5}, {31.5, 41.5}};
    }
    for (auto p : state_.objective_sites)
        paint(static_cast<int>(p.x), static_cast<int>(p.y), Terrain::Ruin);
    for (int i = 0; i < 2; ++i) {
        Owner owner = i ? Owner::Enemy : Owner::Player;
        Vec2 start = i ? map.enemy_start : map.player_start;
        // Clear a safe settlement apron, while keeping the authored river and ridges.
        for (int y = static_cast<int>(start.y) - 5; y <= static_cast<int>(start.y) + 5; ++y)
            for (int x = static_cast<int>(start.x) - 5; x <= static_cast<int>(start.x) + 5; ++x)
                paint(x, y, Terrain::Grass);
        add_building(owner, BuildingKind::Castle, start, true);
        auto &faction = state_.factions[i];
        faction.resources = resources(450, 300, 240, 180, 140);
        faction.capacity = resources(1200, 1200, 1200, 1200, 450);
        for (int k = 0; k < 8; ++k)
            add_unit(owner, UnitKind::Worker, {start.x + (k % 4) - 1.5, start.y + 3 + (k / 4)});
        for (int k = 0; k < 3; ++k)
            add_unit(owner, UnitKind::Militia, {start.x + (k - 1), start.y - 3.5});
        add_unit(owner, UnitKind::Wizard, {start.x + 3.5, start.y});
        for (auto kind : {ResourceKind::Wood, ResourceKind::Stone, ResourceKind::Gold}) {
            Vec2 p = start;
            if (kind == ResourceKind::Wood) {
                p.x += i ? -7 : 7;
                p.y -= 3;
            }
            if (kind == ResourceKind::Stone) {
                p.x += i ? -6 : 6;
                p.y += 7;
            }
            if (kind == ResourceKind::Gold) {
                p.x += i ? 4 : -4;
                p.y -= 7;
            }
            paint(static_cast<int>(p.x), static_cast<int>(p.y),
                  kind == ResourceKind::Wood ? Terrain::Forest : Terrain::Grass);
            state_.nodes.push_back({state_.next_id++, kind, p, 1800});
        }
    }
    for (int n = 0; n < 36; ++n) {
        Vec2 p{3.5 + static_cast<double>(random() % 58), 3.5 + static_cast<double>(random() % 42)};
        auto t = state_.terrain[static_cast<std::size_t>(static_cast<int>(p.y) * map.width +
                                                         static_cast<int>(p.x))];
        if (t == Terrain::Water || t == Terrain::Rock)
            continue;
        bool near_castle = false;
        for (const auto &b : state_.buildings)
            if (distance(p, b.position) < 6)
                near_castle = true;
        if (near_castle)
            continue;
        auto kind = static_cast<ResourceKind>(n % 3 == 0 ? 1 : n % 3 == 1 ? 3 : 0);
        state_.nodes.push_back(
            {state_.next_id++, kind, p, 800 + static_cast<double>(random() % 600)});
        if (kind == ResourceKind::Wood)
            paint(static_cast<int>(p.x), static_cast<int>(p.y), Terrain::Forest);
    }
    recalculate_factions();
    update_network();
    update_visibility();
    event(Owner::Player, 0,
          "Your Dawnkeep is founded. Secure food and timber, then extend the Threadlight network.");
}

bool Simulation::walkable(Vec2 p, Owner owner) const {
    if (static_cast<unsigned>(owner) > 2)
        return false;
    if (!finite(p) || p.x < 0 || p.y < 0 || p.x >= state_.width || p.y >= state_.height)
        return false;
    int x = static_cast<int>(p.x), y = static_cast<int>(p.y);
    auto t = state_.terrain[static_cast<std::size_t>(y * state_.width + x)];
    if (t == Terrain::Rock)
        return false;
    bool water = t == Terrain::Water;
    for (const auto &e : state_.effects)
        if (e.remaining > 0 && distance(p, e.position) <= e.radius) {
            if (e.kind == SpellKind::RootBarrier)
                return false;
            if (e.kind == SpellKind::LightBridge)
                water = false;
        }
    if (water)
        return false;
    for (const auto &b : state_.buildings)
        if (b.health > 0 && contains(footprint(b.kind, b.position, b.rotation), p)) {
            if (b.kind == BuildingKind::Gate && b.owner == owner && complete(b))
                continue;
            return false;
        }
    return true;
}
std::vector<Vec2> Simulation::find_path(Vec2 from, Vec2 to, Owner owner) const {
    if (static_cast<unsigned>(owner) > 2)
        return {};
    if (!finite(from) || !finite(to) || from.x < 0 || from.y < 0 || from.x >= state_.width ||
        from.y >= state_.height || to.x < 0 || to.y < 0 || to.x >= state_.width ||
        to.y >= state_.height)
        return {};
    const int w = state_.width, h = state_.height, count = w * h;
    std::vector<std::uint8_t> pass(static_cast<std::size_t>(count), 1);
    for (int i = 0; i < count; ++i)
        if (state_.terrain[static_cast<std::size_t>(i)] == Terrain::Water ||
            state_.terrain[static_cast<std::size_t>(i)] == Terrain::Rock)
            pass[static_cast<std::size_t>(i)] = 0;
    // Roots take precedence over bridges regardless of their casting order.
    for (auto kind : {SpellKind::LightBridge, SpellKind::RootBarrier})
        for (const auto &e : state_.effects)
            if (e.remaining > 0 && e.kind == kind) {
                int loX = std::max(0, static_cast<int>(e.position.x - e.radius - 1)),
                    hiX = std::min(w - 1, static_cast<int>(e.position.x + e.radius + 1));
                int loY = std::max(0, static_cast<int>(e.position.y - e.radius - 1)),
                    hiY = std::min(h - 1, static_cast<int>(e.position.y + e.radius + 1));
                for (int y = loY; y <= hiY; ++y)
                    for (int x = loX; x <= hiX; ++x)
                        if (distance({x + 0.5, y + 0.5}, e.position) <= e.radius) {
                            auto idx = static_cast<std::size_t>(y * w + x);
                            if (e.kind == SpellKind::RootBarrier)
                                pass[idx] = 0;
                            else if (state_.terrain[idx] == Terrain::Water)
                                pass[idx] = 1;
                        }
            }
    for (const auto &b : state_.buildings)
        if (b.health > 0) {
            if (b.kind == BuildingKind::Gate && b.owner == owner && complete(b))
                continue;
            Rect r = footprint(b.kind, b.position, b.rotation);
            for (int y = std::max(0, r.y); y < std::min(h, r.y + r.h); ++y)
                for (int x = std::max(0, r.x); x < std::min(w, r.x + r.w); ++x)
                    pass[static_cast<std::size_t>(y * w + x)] = 0;
        }
    int start = static_cast<int>(from.y) * w + static_cast<int>(from.x);
    int goal = static_cast<int>(to.y) * w + static_cast<int>(to.x);
    // Commands aimed at a building approach its perimeter instead of entering it.
    if (!pass[static_cast<std::size_t>(goal)]) {
        double best = 1e30;
        int replacement = -1;
        for (int y = std::max(0, static_cast<int>(to.y) - 5);
             y <= std::min(h - 1, static_cast<int>(to.y) + 5); ++y)
            for (int x = std::max(0, static_cast<int>(to.x) - 5);
                 x <= std::min(w - 1, static_cast<int>(to.x) + 5); ++x) {
                int n = y * w + x;
                if (!pass[static_cast<std::size_t>(n)])
                    continue;
                double score = distance({x + 0.5, y + 0.5}, to) * 10 +
                               distance({x + 0.5, y + 0.5}, from) * 0.01;
                if (score < best) {
                    best = score;
                    replacement = n;
                }
            }
        if (replacement < 0)
            return {};
        goal = replacement;
    }
    pass[static_cast<std::size_t>(start)] = 1;
    struct Entry {
        int f, g, n;
        bool operator<(const Entry &other) const {
            return f != other.f ? f > other.f : n > other.n;
        }
    };
    std::priority_queue<Entry> queue;
    std::vector<int> costs(static_cast<std::size_t>(count), std::numeric_limits<int>::max()),
        previous(static_cast<std::size_t>(count), -1);
    auto heuristic = [&](int n) {
        int dx = std::abs(n % w - goal % w), dy = std::abs(n / w - goal / w);
        return 10 * std::max(dx, dy) + 4 * std::min(dx, dy);
    };
    costs[static_cast<std::size_t>(start)] = 0;
    queue.push({heuristic(start), 0, start});
    constexpr int dx[8] = {1, 0, -1, 0, 1, -1, -1, 1}, dy[8] = {0, 1, 0, -1, 1, 1, -1, -1};
    while (!queue.empty()) {
        auto current = queue.top();
        queue.pop();
        if (current.g != costs[static_cast<std::size_t>(current.n)])
            continue;
        if (current.n == goal)
            break;
        int x = current.n % w, y = current.n / w;
        for (int d = 0; d < 8; ++d) {
            int nx = x + dx[d], ny = y + dy[d];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h)
                continue;
            int n = ny * w + nx;
            if (!pass[static_cast<std::size_t>(n)])
                continue;
            if (d >= 4 && (!pass[static_cast<std::size_t>(y * w + nx)] ||
                           !pass[static_cast<std::size_t>(ny * w + x)]))
                continue;
            int g = current.g + (d < 4 ? 10 : 14);
            if (g >= costs[static_cast<std::size_t>(n)])
                continue;
            costs[static_cast<std::size_t>(n)] = g;
            previous[static_cast<std::size_t>(n)] = current.n;
            queue.push({g + heuristic(n), g, n});
        }
    }
    if (costs[static_cast<std::size_t>(goal)] == std::numeric_limits<int>::max())
        return {};
    std::vector<Vec2> path;
    for (int n = goal; n != start; n = previous[static_cast<std::size_t>(n)]) {
        if (n < 0 || path.size() > static_cast<std::size_t>(count))
            return {};
        path.push_back({n % w + 0.5, n / w + 0.5});
    }
    std::reverse(path.begin(), path.end());
    if (path.empty())
        path.push_back({goal % w + 0.5, goal / w + 0.5});
    return path;
}
void Simulation::order_path(Unit &u, Vec2 destination) {
    u.destination = destination;
    u.path = find_path(u.position, destination, u.owner);
    u.path_index = 0;
    u.repath_timer = 2;
}
bool Simulation::has_light(Owner owner, Vec2 p) const {
    if (static_cast<unsigned>(owner) >= 2 || !finite(p))
        return false;
    for (const auto &b : state_.buildings)
        if (b.owner == owner && complete(b) && b.connected && light_node(b.kind)) {
            double radius = definition(b.kind).light_radius;
            for (const auto &e : state_.effects)
                if (e.owner == owner && e.kind == SpellKind::Amplify && e.remaining > 0 &&
                    distance(b.position, e.position) < e.radius)
                    radius += 5;
            if (distance(b.position, p) <= radius)
                return true;
        }
    return false;
}
bool Simulation::is_visible(Owner owner, Vec2 p) const {
    if (static_cast<unsigned>(owner) >= 2 || !finite(p) || p.x < 0 || p.y < 0 ||
        p.x >= state_.width || p.y >= state_.height)
        return false;
    auto idx =
        static_cast<std::size_t>(static_cast<int>(p.y) * state_.width + static_cast<int>(p.x));
    return idx < state_.visible[side(owner)].size() && state_.visible[side(owner)][idx] != 0;
}
bool Simulation::is_unit_visible(Owner viewer, Id id) const {
    const auto *target = unit(id);
    if (!target || viewer == Owner::Neutral || static_cast<unsigned>(viewer) > 2)
        return false;
    if (target->owner == viewer)
        return true;
    if (!is_visible(viewer, target->position))
        return false;
    return !concealed(state_, *target, viewer);
}
double Simulation::day_fraction() const {
    return std::fmod(static_cast<double>(state_.tick) * FixedStep, 240.0) / 240.0;
}
CommandResult Simulation::can_build(Owner owner, BuildingKind kind, Vec2 p,
                                    std::uint8_t rotation) const {
    if (state_.outcome != MatchOutcome::Playing)
        return reject("The match has ended.");
    if (owner == Owner::Neutral || static_cast<unsigned>(owner) > 2 ||
        static_cast<unsigned>(kind) >= 19 || rotation > 3 || !finite(p))
        return reject("Invalid construction parameters.");
    if (p.x < 1 || p.y < 1 || p.x >= state_.width - 1 || p.y >= state_.height - 1)
        return reject("Keep the footprint inside the map.");
    if (state_.buildings.size() >= MaximumEntities)
        return reject("Building limit reached.");
    Rect rect = footprint(kind, centre(p), rotation);
    if (rect.x < 1 || rect.y < 1 || rect.x + rect.w >= state_.width ||
        rect.y + rect.h >= state_.height)
        return reject("Keep the footprint inside the map.");
    const auto &faction = state_.factions[side(owner)];
    if (!faction.resources.can_afford(definition(kind).cost))
        return reject("Not enough resources.");
    for (int y = rect.y; y < rect.y + rect.h; ++y)
        for (int x = rect.x; x < rect.x + rect.w; ++x) {
            Terrain t = state_.terrain[static_cast<std::size_t>(y * state_.width + x)];
            if (t == Terrain::Water || t == Terrain::Rock)
                return reject("The terrain cannot support this building.");
            if (!state_.explored[side(owner)].empty() &&
                !state_.explored[side(owner)][static_cast<std::size_t>(y * state_.width + x)])
                return reject("Explore this area before building.");
        }
    for (const auto &b : state_.buildings)
        if (b.health > 0 && overlap(rect, footprint(b.kind, b.position, b.rotation)))
            return reject("The footprint overlaps another building.");
    for (const auto &n : state_.nodes)
        if (n.remaining > 0 && contains(rect, n.position))
            return reject("Keep access to the resource deposit.");
    for (const auto &e : state_.effects)
        if (e.remaining > 0 && e.kind == SpellKind::RootBarrier &&
            distance(e.position, centre(p)) < e.radius + std::max(rect.w, rect.h) * 0.5)
            return reject("Roots block this site.");
    bool worker = false, near = false;
    for (const auto &u : state_.units)
        if (u.health > 0 && u.owner == owner &&
            (u.kind == UnitKind::Worker || u.kind == UnitKind::Engineer))
            worker = true;
    for (const auto &b : state_.buildings)
        if (b.owner == owner && complete(b) && (storage(b.kind) || light_node(b.kind)) &&
            distance(b.position, p) <= 18)
            near = true;
    if (!worker)
        return reject("A Hearthfolk worker or Runesmith is required.");
    if (!near)
        return reject("Build within 18 cells of a Dawnkeep, depot, or light node.");
    if (definition(kind).requires_light && !has_light(owner, p))
        return reject("This building needs a connected Threadlight network.");
    return accept();
}

CommandResult Simulation::issue(const Command &c) {
    if (state_.outcome != MatchOutcome::Playing)
        return reject("The match has ended.");
    if (static_cast<unsigned>(c.owner) >= 2 ||
        static_cast<unsigned>(c.type) > static_cast<unsigned>(CommandType::SetAutomaticGrowth) ||
        !finite(c.position) || c.ids.size() > MaximumEntities)
        return reject("Invalid command parameters.");
    auto &faction = state_.factions[side(c.owner)];
    if (c.type == CommandType::SetAutomaticGrowth) {
        if (!valid_growth_policy(c.automatic_growth) || c.target_id || !c.ids.empty())
            return reject("Invalid automatic growth policy parameters.");
        faction.automatic_growth = c.automatic_growth;
        if (!faction.automatic_growth.enabled)
            faction.housing_demand_ticks = 0;
        return accept();
    }
    if (c.type == CommandType::Surrender) {
        state_.outcome =
            c.owner == Owner::Player ? MatchOutcome::EnemyVictory : MatchOutcome::PlayerVictory;
        for (auto &f : state_.factions)
            f.housing_demand_ticks = 0;
        event(c.owner, 0, "The kingdom surrendered.");
        return accept();
    }
    if (c.type == CommandType::Build) {
        auto result = can_build(c.owner, c.building, c.position, c.rotation);
        if (!result)
            return result;
        // Do not trap units underneath a new foundation: move them to nearby land.
        Rect rect = footprint(c.building, centre(c.position), c.rotation);
        std::vector<std::pair<Id, Vec2>> evacuations;
        for (const auto &u : state_.units)
            if (u.health > 0 && contains(rect, u.position)) {
                if (u.owner != c.owner)
                    return reject("An enemy occupies the foundation.");
                Vec2 best{};
                double score = 1e30;
                for (int y = rect.y - 1; y <= rect.y + rect.h; ++y)
                    for (int x = rect.x - 1; x <= rect.x + rect.w; ++x) {
                        Vec2 p{x + 0.5, y + 0.5};
                        if (!contains(rect, p) && walkable(p, u.owner) &&
                            distance(p, u.position) < score) {
                            best = p;
                            score = distance(p, u.position);
                        }
                    }
                if (score == 1e30)
                    return reject("Workers need a safe exit from this foundation.");
                evacuations.push_back({u.id, best});
            }
        subtract(faction.resources, definition(c.building).cost);
        Id id = add_building(c.owner, c.building, c.position, false);
        mutable_building(id)->rotation = c.rotation;
        for (const auto &move : evacuations)
            if (auto *u = mutable_unit(move.first)) {
                u->position = move.second;
                clear_path(*u);
            }
        // Assign up to two nearby builders. Existing construction crews keep their jobs.
        std::vector<std::pair<double, Id>> candidates;
        for (const auto &u : state_.units)
            if (u.owner == c.owner && u.health > 0 &&
                (u.kind == UnitKind::Worker || u.kind == UnitKind::Engineer) &&
                u.order != OrderType::Build && u.order != OrderType::Repair)
                candidates.push_back({distance(u.position, c.position), u.id});
        std::sort(candidates.begin(), candidates.end());
        for (std::size_t n = 0; n < std::min<std::size_t>(2, candidates.size()); ++n)
            if (auto *u = mutable_unit(candidates[n].second)) {
                u->order = OrderType::Build;
                u->target_id = id;
                u->job_id = 0;
                order_path(*u, c.position);
            }
        event(c.owner, id, std::string(definition(c.building).name) + " foundation placed.");
        recalculate_factions();
        return accept(id);
    }
    if (c.type == CommandType::Train) {
        if (static_cast<unsigned>(c.unit) >= 12)
            return reject("Unknown unit type.");
        Id recruiter = c.target_id;
        if (!recruiter && !c.ids.empty())
            recruiter = c.ids.front();
        auto *b = mutable_building(recruiter);
        if (!b || b->owner != c.owner || !complete(*b))
            return reject("Select a completed recruiting building.");
        const auto &d = definition(c.unit);
        if (b->kind != d.recruited_at)
            return reject("This building cannot recruit that unit.");
        if (b->kind == BuildingKind::MageTower && !b->connected)
            return reject("Reconnect the Starweaver Spire to light.");
        if (b->train_queue.size() >= 8 || state_.units.size() >= MaximumEntities)
            return reject("Recruitment queue is full.");
        if (c.unit == UnitKind::Cavalry || c.unit == UnitKind::Swordsman) {
            bool forge = false;
            for (const auto &v : state_.buildings)
                if (v.owner == c.owner && v.kind == BuildingKind::Forge && complete(v))
                    forge = true;
            if (!forge)
                return reject("Build an Ember Forge first.");
        }
        if (faction.population + d.population > faction.population_limit)
            return reject("Build more Hearth Cottages for population capacity.");
        if (!faction.resources.can_afford(d.cost))
            return reject("Not enough resources for recruitment.");
        subtract(faction.resources, d.cost);
        b->train_queue.push_back(c.unit);
        recalculate_factions();
        return accept();
    }
    if (c.type == CommandType::Upgrade || c.type == CommandType::Demolish ||
        c.type == CommandType::SetRally) {
        Id id = c.target_id;
        if (!id && !c.ids.empty())
            id = c.ids.front();
        auto *b = mutable_building(id);
        if (!b || b->owner != c.owner)
            return reject("Select one of your buildings.");
        if (c.type == CommandType::SetRally) {
            if (c.position.x < 0 || c.position.y < 0 || c.position.x >= state_.width ||
                c.position.y >= state_.height)
                return reject("Rally point is outside the map.");
            b->rally = c.position;
            return accept();
        }
        if (c.type == CommandType::Upgrade) {
            if (!complete(*b) || b->level >= 3)
                return reject("This building cannot be upgraded further.");
            if (!upgrade_building(*b))
                return reject("Not enough resources for the upgrade.");
            return accept();
        }
        if (b->kind == BuildingKind::Castle)
            return reject("The Dawnkeep cannot be demolished. Use Surrender to concede.");
        for (auto kind : b->train_queue)
            for (auto resource : {ResourceKind::Wood, ResourceKind::Stone, ResourceKind::Food,
                                  ResourceKind::Gold, ResourceKind::Mana})
                deposit(faction, resource, definition(kind).cost.get(resource));
        for (auto resource : {ResourceKind::Wood, ResourceKind::Stone, ResourceKind::Food,
                              ResourceKind::Gold, ResourceKind::Mana})
            deposit(faction, resource,
                    definition(b->kind).cost.get(resource) * (complete(*b) ? 0.35 : 0.75));
        b->health = 0;
        event(c.owner, id, "Building dismantled; recoverable supplies returned.");
        update_network();
        recalculate_factions();
        return accept();
    }
    if (c.type == CommandType::Research) {
        if (static_cast<unsigned>(c.technology) >= 5)
            return reject("Unknown technology.");
        std::size_t index = static_cast<std::size_t>(c.technology);
        if (faction.technologies[index] >= 2 || faction.research != Technology::Count)
            return reject("A technology is already being researched, or this one is complete.");
        BuildingKind required =
            c.technology == Technology::Arcana ? BuildingKind::MageTower
            : c.technology == Technology::Steel || c.technology == Technology::Masonry
                ? BuildingKind::Forge
                : BuildingKind::Mill;
        bool available = false;
        for (const auto &b : state_.buildings)
            if (b.owner == c.owner && b.kind == required && complete(b) &&
                (required != BuildingKind::MageTower || b.connected))
                available = true;
        if (!available)
            return reject(std::string("Research requires a ") +
                          std::string(definition(required).name) + ".");
        int level = faction.technologies[index] + 1;
        Resources cost = resources(45 * level, 25 * level, 0, 50 * level,
                                   c.technology == Technology::Arcana ? 25 * level : 0);
        if (!faction.resources.can_afford(cost))
            return reject("Not enough resources for research.");
        subtract(faction.resources, cost);
        faction.research = c.technology;
        faction.research_timer = 35 * level;
        return accept();
    }
    if (c.type == CommandType::Trade) {
        if (static_cast<unsigned>(c.resource) >= 5 || c.resource == ResourceKind::Mana ||
            !std::isfinite(c.amount) || c.amount <= 0 || c.amount > 500)
            return reject("Choose 1 to 500 units of a physical resource to sell.");
        bool available = false;
        for (const auto &b : state_.buildings)
            if (b.owner == c.owner && b.kind == BuildingKind::Market && complete(b))
                available = true;
        if (!available)
            return reject("Build a Lantern Market to trade.");
        if (c.resource == ResourceKind::Gold) {
            if (faction.resources.gold + 1e-8 < c.amount)
                return reject("Not enough gold.");
            faction.resources.gold -= c.amount;
            deposit(faction, ResourceKind::Food, c.amount * 2.5);
        } else {
            if (faction.resources.get(c.resource) + 1e-8 < c.amount)
                return reject("Not enough goods to sell.");
            faction.resources.add(c.resource, -c.amount);
            deposit(faction, ResourceKind::Gold,
                    c.amount * (c.resource == ResourceKind::Stone ? 0.65 : 0.45));
        }
        return accept();
    }
    if (c.type == CommandType::CastSpell) {
        if (static_cast<unsigned>(c.spell) >= 9)
            return reject("Unknown spell.");
        const auto &d = definition(c.spell);
        Unit *caster = nullptr;
        if (!c.ids.empty()) {
            caster = mutable_unit(c.ids.front());
            if (!caster || caster->owner != c.owner || caster->kind != UnitKind::Wizard)
                return reject("Select a Wildlight Weaver.");
        } else
            for (auto &u : state_.units)
                if (u.owner == c.owner && u.health > 0 && u.kind == UnitKind::Wizard &&
                    distance(u.position, c.position) <= d.cast_range) {
                    caster = &u;
                    break;
                }
        if (!caster)
            return reject("A Wildlight Weaver must be within casting range.");
        if (c.position.x < 1 || c.position.y < 1 || c.position.x >= state_.width - 1 ||
            c.position.y >= state_.height - 1 ||
            distance(caster->position, c.position) > d.cast_range)
            return reject("The spell is outside the Weaver's casting range.");
        if (caster->spell_cooldowns[static_cast<std::size_t>(c.spell)] > 0)
            return reject("This spell is cooling down.");
        double mana = d.mana * (1 - 0.1 * faction.technologies[4]);
        if (faction.resources.mana + 1e-8 < mana)
            return reject("Not enough magical energy.");
        if (!is_visible(c.owner, c.position))
            return reject("The Weaver needs sight of the spell target.");
        Vec2 position = centre(c.position);
        if (c.spell == SpellKind::LightBridge) {
            double best = 2.6;
            bool found = false;
            for (auto site : state_.crossing_sites)
                if (distance(site, c.position) < best) {
                    position = site;
                    best = distance(site, c.position);
                    found = true;
                }
            if (!found)
                return reject("Threadway needs one of the marked river crossing sites.");
            bool water = false;
            for (int y = 0; y < state_.height; ++y)
                for (int x = 0; x < state_.width; ++x)
                    if (state_.terrain[static_cast<std::size_t>(y * state_.width + x)] ==
                            Terrain::Water &&
                        distance({x + 0.5, y + 0.5}, position) <= d.radius)
                        water = true;
            if (!water)
                return reject("This crossing has no water to bridge.");
        }
        if (c.spell == SpellKind::RootBarrier) {
            if (!walkable(position, c.owner))
                return reject("Briar Rampart must grow on open land.");
            for (const auto &b : state_.buildings)
                if (b.health > 0 && building_distance(b, position) <= d.radius)
                    return reject("Keep roots clear of building foundations.");
            for (const auto &u : state_.units)
                if (u.health > 0 && distance(u.position, position) <= d.radius + 0.3)
                    return reject("Roots cannot trap a unit inside their footprint.");
        }
        if (c.spell == SpellKind::Amplify) {
            const auto *target = building(c.target_id);
            if (!target || target->owner != c.owner || !complete(*target) ||
                target->kind != BuildingKind::Beacon || !target->connected ||
                distance(target->position, position) > d.radius)
                return reject("Target a connected friendly Threadlight Beacon.");
            position = target->position;
        }
        if (state_.effects.size() >= MaximumEntities ||
            (c.spell == SpellKind::Illusion && state_.units.size() + 3 >= MaximumEntities))
            return reject("Spell effect limit reached.");
        Id caster_id = caster->id;
        faction.resources.mana -= mana;
        caster->spell_cooldowns[static_cast<std::size_t>(c.spell)] = d.cooldown;
        Id effect_id = state_.next_id++;
        state_.effects.push_back({effect_id, c.owner, c.spell, position, d.duration, d.radius,
                                  c.spell == SpellKind::Amplify ? c.target_id : 0});
        if (c.spell == SpellKind::Heal) {
            for (auto &u : state_.units)
                if (u.owner == c.owner && u.health > 0 &&
                    distance(u.position, position) <= d.radius)
                    u.health = std::min(max_health(u), u.health + 45);
            for (auto &b : state_.buildings)
                if (b.owner == c.owner && b.health > 0 &&
                    distance(b.position, position) <= d.radius)
                    b.health = std::min(max_health(b), b.health + 65);
        }
        if (c.spell == SpellKind::Disrupt)
            for (auto &e : state_.effects)
                if (e.owner != c.owner && e.remaining > 0 &&
                    distance(e.position, position) <= d.radius &&
                    (e.kind == SpellKind::RootBarrier || e.kind == SpellKind::Shield ||
                     e.kind == SpellKind::Veil))
                    e.remaining = 0;
        if (c.spell == SpellKind::Illusion)
            for (int n = 0; n < 3; ++n) {
                Vec2 p{position.x + (n - 1) * 0.8, position.y + 0.8};
                if (!walkable(p, c.owner))
                    continue;
                Id id = add_unit(c.owner, UnitKind::Militia, p);
                auto *u = mutable_unit(id);
                u->illusion_seconds = d.duration;
                u->health = 1;
                u->stance = Stance::Aggressive;
            }
        event(c.owner, caster_id, std::string(d.name) + " cast.");
        update_network();
        return accept(effect_id);
    }
    if (c.ids.empty())
        return reject("Select units first.");
    std::vector<Id> ids;
    ids.reserve(c.ids.size());
    for (auto id : c.ids) {
        const auto *u = unit(id);
        if (!u || u->owner != c.owner)
            return reject("Every selected unit must belong to your kingdom.");
        if (std::find(ids.begin(), ids.end(), id) == ids.end())
            ids.push_back(id);
    }
    if (c.type == CommandType::SetStance && static_cast<unsigned>(c.stance) > 2)
        return reject("Unknown stance.");
    const Building *target_building = building(c.target_id);
    const Unit *target_unit = unit(c.target_id);
    const ResourceNode *target_node = node(c.target_id);
    if (c.type == CommandType::Attack) {
        if ((!target_building && !target_unit) ||
            (target_building && target_building->owner == c.owner) ||
            (target_unit && target_unit->owner == c.owner))
            return reject("Choose a hostile unit or building.");
        if (!is_visible(c.owner,
                        target_building ? target_building->position : target_unit->position))
            return reject("The target is outside your sight.");
        if (target_unit && !is_unit_visible(c.owner, target_unit->id))
            return reject("Mothveil conceals this unit. Scouts reveal it within 2.5 cells.");
    }
    if (c.type == CommandType::Gather) {
        if ((!target_node || target_node->remaining <= 0) &&
            (!target_building || target_building->owner != c.owner ||
             target_building->kind != BuildingKind::Farm || !complete(*target_building)))
            return reject("Choose an available resource deposit or friendly farm.");
        if (target_node && !is_visible(c.owner, target_node->position))
            return reject("Explore the resource deposit before assigning a harvest crew.");
        for (auto id : ids)
            if (unit(id)->kind != UnitKind::Worker)
                return reject("Only Hearthfolk can harvest resources.");
    }
    if (c.type == CommandType::Repair) {
        if (!target_building || target_building->owner != c.owner)
            return reject("Choose one of your buildings to repair.");
        for (auto id : ids)
            if (unit(id)->kind != UnitKind::Worker && unit(id)->kind != UnitKind::Engineer)
                return reject("Repairs require Hearthfolk or a Runesmith.");
    }
    if (c.type == CommandType::Move || c.type == CommandType::Patrol) {
        if (c.position.x < 1 || c.position.y < 1 || c.position.x >= state_.width - 1 ||
            c.position.y >= state_.height - 1)
            return reject("The destination is outside the map.");
        bool reachable = false;
        for (auto id : ids)
            if (!find_path(unit(id)->position, c.position, c.owner).empty())
                reachable = true;
        if (!reachable)
            return reject("No selected unit can reach this destination.");
    }
    for (std::size_t i = 0; i < ids.size(); ++i) {
        auto *u = mutable_unit(ids[i]);
        if (!u)
            continue;
        if (c.type == CommandType::SetStance) {
            u->stance = c.stance;
            continue;
        }
        u->job_id = 0;
        u->patrolling = c.type == CommandType::Patrol;
        if (c.type == CommandType::Stop) {
            u->order = OrderType::Idle;
            u->automatic_work = false;
            u->target_id = 0;
            clear_path(*u);
            continue;
        }
        if (c.type == CommandType::Move || c.type == CommandType::Patrol) {
            u->automatic_work = true;
            u->order = c.type == CommandType::Move ? OrderType::Move : OrderType::Patrol;
            u->target_id = 0;
            u->patrol_origin = u->position;
            int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(ids.size()))));
            Vec2 p{c.position.x + (static_cast<int>(i) % columns - (columns - 1) * 0.5) * 0.8,
                   c.position.y + (static_cast<int>(i) / columns - (columns - 1) * 0.5) * 0.8};
            if (!walkable(p, c.owner))
                p = c.position;
            u->patrol_destination = p;
            order_path(*u, p);
        } else if (c.type == CommandType::Attack) {
            u->order = OrderType::Attack;
            u->target_id = c.target_id;
            order_path(*u, target_building ? target_building->position : target_unit->position);
        } else if (c.type == CommandType::Gather) {
            u->automatic_work = false;
            u->order = u->cargo > 0 ? OrderType::ReturnCargo : OrderType::Gather;
            u->target_id = c.target_id;
            u->job_id = target_building ? target_building->id : 0;
            order_path(*u, target_node ? target_node->position : target_building->position);
        } else if (c.type == CommandType::Repair) {
            u->order = target_building->progress < 1 ? OrderType::Build : OrderType::Repair;
            u->target_id = c.target_id;
            order_path(*u, target_building->position);
        }
    }
    return accept();
}

void Simulation::recalculate_factions() {
    for (auto &f : state_.factions) {
        f.population = 0;
        f.population_limit = 0;
        f.capacity = resources(300, 300, 300, 300, 180);
    }
    for (const auto &b : state_.buildings)
        if (b.health > 0 && b.owner != Owner::Neutral) {
            auto &f = state_.factions[side(b.owner)];
            const auto &d = definition(b.kind);
            if (complete(b)) {
                f.population_limit += d.population * b.level;
                if (d.storage) {
                    f.capacity.wood += d.storage * b.level;
                    f.capacity.stone += d.storage * b.level;
                    f.capacity.food += d.storage * b.level;
                    f.capacity.gold += d.storage * b.level;
                }
                if (light_node(b.kind))
                    f.capacity.mana += b.kind == BuildingKind::Castle ? 270 : 60 * b.level;
            }
            for (auto kind : b.train_queue)
                f.population += definition(kind).population;
        }
    for (const auto &u : state_.units)
        if (u.health > 0 && u.owner != Owner::Neutral && u.illusion_seconds <= 0)
            state_.factions[side(u.owner)].population += definition(u.kind).population;
    for (auto &f : state_.factions)
        for (auto k : {ResourceKind::Wood, ResourceKind::Stone, ResourceKind::Food,
                       ResourceKind::Gold, ResourceKind::Mana})
            if (f.resources.get(k) > f.capacity.get(k))
                f.resources.add(k, f.capacity.get(k) - f.resources.get(k));
}
bool Simulation::upgrade_building(Building &building) {
    if (!complete(building) || building.level >= 3)
        return false;
    auto &faction = state_.factions[side(building.owner)];
    const Resources cost = building_upgrade_cost(building);
    if (!faction.resources.can_afford(cost))
        return false;
    const double health_fraction = building.health / max_health(building);
    subtract(faction.resources, cost);
    // The shared affordability tolerance must not leave a negative stock balance.
    for (auto kind : {ResourceKind::Wood, ResourceKind::Stone, ResourceKind::Food,
                      ResourceKind::Gold, ResourceKind::Mana})
        if (faction.resources.get(kind) < 0)
            faction.resources.add(kind, -faction.resources.get(kind));
    ++building.level;
    building.health = max_health(building) * health_fraction;
    building.harvested_food = 0;
    if (grows_automatically(building.kind))
        faction.automatic_growth_cooldown_ticks = 150;
    event(building.owner, building.id,
          "Building upgraded to level " + std::to_string(building.level) + ".");
    recalculate_factions();
    return true;
}
std::string Simulation::automatic_growth_reason(const Building &building) const {
    if (!grows_automatically(building.kind))
        return "Manual upgrades only";
    if (state_.outcome != MatchOutcome::Playing)
        return "Match ended";
    const auto &faction = state_.factions[side(building.owner)];
    if (!faction.automatic_growth.enabled)
        return "Automatic growth disabled";
    if (building.level >= 3)
        return "Maximum level";
    if (!complete(building))
        return "Construction unfinished";
    if (building.health < max_health(building) * 0.9)
        return "Repair needed (90% health required)";
    if (building.last_damage_tick &&
        (building.last_damage_tick > state_.tick || state_.tick - building.last_damage_tick < 100))
        return "Recent damage (10 seconds without damage required)";
    if (building.kind == BuildingKind::House && faction.housing_demand_ticks < 100)
        return "Housing demand (80% capacity for 10 seconds required)";
    if (building.kind == BuildingKind::Farm && building.harvested_food < farm_milestone(building))
        return "Farm production (" + std::to_string(static_cast<int>(building.harvested_food)) +
               "/" + std::to_string(static_cast<int>(farm_milestone(building))) + " food)";
    if (faction.automatic_growth_cooldown_ticks)
        return "Automatic growth cooldown";
    const Resources cost = building_upgrade_cost(building);
    if (!faction.resources.can_afford(cost) ||
        faction.resources.wood - cost.wood + 1e-8 < faction.automatic_growth.protected_wood ||
        faction.resources.stone - cost.stone + 1e-8 < faction.automatic_growth.protected_stone)
        return "Protected reserve (deposited wood and stone required)";
    return "Ready";
}
std::string Simulation::automatic_growth_status(Id id) const {
    const auto *candidate = building(id);
    return candidate ? automatic_growth_reason(*candidate) : "Building unavailable";
}
void Simulation::update_automatic_growth() {
    for (auto &faction : state_.factions) {
        const bool pressure = faction.population_limit > 0 &&
                              static_cast<std::int64_t>(faction.population) * 5 >=
                                  static_cast<std::int64_t>(faction.population_limit) * 4;
        if (!faction.automatic_growth.enabled || state_.outcome != MatchOutcome::Playing || !pressure)
            faction.housing_demand_ticks = 0;
        else
            faction.housing_demand_ticks = std::min(100u, faction.housing_demand_ticks + 1);
    }
    if (state_.outcome != MatchOutcome::Playing || state_.tick % 10 != 0)
        return;
    for (Owner owner : {Owner::Player, Owner::Enemy}) {
        Building *candidate = nullptr;
        for (auto &building : state_.buildings) {
            if (building.owner != owner || automatic_growth_reason(building) != "Ready")
                continue;
            if (!candidate || (building.kind == BuildingKind::House && candidate->kind != BuildingKind::House) ||
                (building.kind == candidate->kind && building.id < candidate->id))
                candidate = &building;
        }
        if (candidate)
            upgrade_building(*candidate);
    }
}
void Simulation::update_network() {
    for (auto &b : state_.buildings)
        b.connected = false;
    auto disrupted = [&](const Building &b) {
        for (const auto &e : state_.effects)
            if (e.remaining > 0 && e.kind == SpellKind::Disrupt && e.owner != b.owner &&
                distance(e.position, b.position) <= e.radius)
                return true;
        return false;
    };
    auto radius = [&](const Building &b) {
        double r = definition(b.kind).light_radius;
        for (const auto &e : state_.effects)
            if (e.remaining > 0 && e.kind == SpellKind::Amplify && e.owner == b.owner &&
                distance(e.position, b.position) <= e.radius)
                r += 5;
        return r;
    };
    for (auto &b : state_.buildings)
        if (b.kind == BuildingKind::Castle && complete(b) && !disrupted(b))
            b.connected = true;
    bool changed = true;
    int iterations = 0;
    while (changed && iterations++ < static_cast<int>(state_.buildings.size())) {
        changed = false;
        for (auto &b : state_.buildings)
            if (!b.connected && complete(b) && light_node(b.kind) && !disrupted(b)) {
                for (const auto &a : state_.buildings)
                    if (a.owner == b.owner && a.connected && light_node(a.kind) &&
                        distance(a.position, b.position) <= radius(a)) {
                        b.connected = true;
                        changed = true;
                        break;
                    }
            }
    }
    for (auto &b : state_.buildings)
        if (complete(b) && !light_node(b.kind) && !disrupted(b))
            b.connected = has_light(b.owner, b.position);
}
void Simulation::assign_jobs() {
    // Automatic crews leave saturated industries and staff newly completed
    // facilities. Explicit player harvest orders retain their chosen deposit.
    for (auto &u : state_.units)
        if (u.health > 0 && u.kind == UnitKind::Worker && u.automatic_work && u.cargo <= 1e-8 &&
            u.order == OrderType::Gather) {
            auto &f = state_.factions[side(u.owner)];
            ResourceKind kind =
                u.job_id && building(u.job_id) ? output(building(u.job_id)->kind) : u.cargo_kind;
            if (f.resources.get(kind) > f.capacity.get(kind) * 0.88 || u.job_id == 0) {
                u.order = OrderType::Idle;
                u.job_id = 0;
                u.target_id = 0;
                clear_path(u);
            }
        }
    for (auto &u : state_.units)
        if (u.health > 0 && (u.kind == UnitKind::Worker || u.kind == UnitKind::Engineer) &&
            u.order == OrderType::Idle) {
            if (!u.automatic_work)
                continue;
            if (u.cargo > 0) {
                u.order = OrderType::ReturnCargo;
                clear_path(u);
                continue;
            }
            Id construction = 0;
            double best = 1e30;
            for (const auto &b : state_.buildings)
                if (b.owner == u.owner && b.health > 0 && b.progress < 1) {
                    int crew = 0;
                    for (const auto &other : state_.units)
                        if (other.health > 0 && other.order == OrderType::Build &&
                            other.target_id == b.id)
                            ++crew;
                    if (crew < 2 && distance(u.position, b.position) < best) {
                        best = distance(u.position, b.position);
                        construction = b.id;
                    }
                }
            if (construction) {
                u.order = OrderType::Build;
                u.target_id = construction;
                u.job_id = 0;
                order_path(u, building(construction)->position);
                continue;
            }
            if (u.kind == UnitKind::Engineer)
                continue;
            Id facility = 0;
            best = 1e30;
            for (const auto &b : state_.buildings)
                if (b.owner == u.owner && complete(b) && producer(b.kind)) {
                    const auto &f = state_.factions[side(u.owner)];
                    if (f.resources.get(output(b.kind)) >= f.capacity.get(output(b.kind)) * 0.9)
                        continue;
                    int count = 0;
                    for (const auto &other : state_.units)
                        if (other.health > 0 && other.job_id == b.id)
                            ++count;
                    if (count >= definition(b.kind).worker_slots)
                        continue;
                    if (b.kind != BuildingKind::Farm) {
                        bool available = false;
                        for (const auto &n : state_.nodes)
                            if (n.remaining > 0 && n.kind == output(b.kind) &&
                                distance(n.position, b.position) <= 11)
                                available = true;
                        if (!available)
                            continue;
                    }
                    double stock = f.resources.get(output(b.kind));
                    double weight = b.kind == BuildingKind::Farm ? 0.55 : 1;
                    if (stock < 100)
                        weight *= 0.25;
                    if (distance(u.position, b.position) * weight < best) {
                        best = distance(u.position, b.position) * weight;
                        facility = b.id;
                    }
                }
            const Building *b = facility ? building(facility) : nullptr;
            if (b && b->kind == BuildingKind::Farm) {
                u.job_id = b->id;
                u.target_id = b->id;
                u.order = OrderType::Gather;
                order_path(u, b->position);
                continue;
            }
            ResourceKind wanted = ResourceKind::Wood;
            const auto &stock = state_.factions[side(u.owner)].resources;
            if (b)
                wanted = output(b->kind);
            else if (stock.wood > 160 && stock.stone < 180)
                wanted = ResourceKind::Stone;
            else if (stock.wood > 180 && stock.gold < 120)
                wanted = ResourceKind::Gold;
            // Idle workers share fallback deposits instead of all visiting one tree.
            Id target = 0;
            best = 1e30;
            for (const auto &n : state_.nodes)
                if (n.remaining > 0 && n.kind == wanted &&
                    (!b || distance(n.position, b->position) <= 11)) {
                    const auto &f = state_.factions[side(u.owner)];
                    if (f.resources.get(n.kind) >= f.capacity.get(n.kind) * 0.9)
                        continue;
                    auto cell =
                        static_cast<std::size_t>(static_cast<int>(n.position.y) * state_.width +
                                                 static_cast<int>(n.position.x));
                    if (!state_.explored[side(u.owner)][cell])
                        continue;
                    int assigned = 0;
                    for (const auto &other : state_.units)
                        if (other.health > 0 && other.target_id == n.id)
                            ++assigned;
                    double score = distance(u.position, n.position) + assigned * 2.0;
                    if (score < best) {
                        best = score;
                        target = n.id;
                    }
                }
            if (target) {
                u.job_id = facility;
                u.target_id = target;
                u.order = OrderType::Gather;
                order_path(u, node(target)->position);
            }
        }
}
void Simulation::update_economy() {
    state_.upkeep_timer += FixedStep;
    if (state_.upkeep_timer >= 5) {
        state_.upkeep_timer -= 5;
        for (auto &f : state_.factions) {
            double food = 0.35 * f.population;
            if (f.resources.food + 1e-8 >= food) {
                f.resources.food -= food;
                f.morale = std::min(100.0, f.morale + 2.0);
            } else {
                f.resources.food = 0;
                f.morale = std::max(20.0, f.morale - 6.0);
            }
            if (f.population > f.population_limit)
                f.morale = std::max(20.0, f.morale - 3.0);
        }
    }
    for (auto &f : state_.factions)
        if (f.research != Technology::Count) {
            f.research_timer = std::max(0.0, f.research_timer - FixedStep);
            if (f.research_timer <= 0) {
                ++f.technologies[static_cast<std::size_t>(f.research)];
                f.research = Technology::Count;
            }
        }
    struct Recruit {
        Owner owner;
        UnitKind kind;
        Vec2 position, rally;
    };
    std::vector<Recruit> recruits;
    for (auto &b : state_.buildings)
        if (complete(b)) {
            auto &faction = state_.factions[side(b.owner)];
            if (b.connected) {
                double yield = b.kind == BuildingKind::Castle      ? 0.35
                               : b.kind == BuildingKind::Beacon    ? 0.2
                               : b.kind == BuildingKind::Sanctuary ? 1.2
                                                                   : 0;
                deposit(faction, ResourceKind::Mana,
                        yield * b.level * FixedStep * (1 + 0.2 * faction.technologies[4]));
            }
            if (!b.train_queue.empty() && (!definition(b.kind).requires_light || b.connected)) {
                b.train_timer += FixedStep * (0.65 + 0.35 * faction.morale / 100.0);
                auto kind = b.train_queue.front();
                if (b.train_timer + 1e-8 >= definition(kind).train_seconds &&
                    state_.units.size() + recruits.size() < MaximumEntities) {
                    Rect r = footprint(b.kind, b.position, b.rotation);
                    Vec2 spawn{};
                    bool found = false;
                    double best = 1e30;
                    for (int y = r.y - 1; y <= r.y + r.h; ++y)
                        for (int x = r.x - 1; x <= r.x + r.w; ++x) {
                            Vec2 p{x + 0.5, y + 0.5};
                            if (walkable(p, b.owner) && distance(p, b.rally) < best) {
                                best = distance(p, b.rally);
                                spawn = p;
                                found = true;
                            }
                        }
                    if (found) {
                        b.train_queue.erase(b.train_queue.begin());
                        b.train_timer = 0;
                        recruits.push_back({b.owner, kind, spawn, b.rally});
                        ++faction.units_recruited;
                    }
                }
            }
            if (b.kind == BuildingKind::Watchtower || b.kind == BuildingKind::Castle) {
                b.attack_timer = std::max(0.0, b.attack_timer - FixedStep);
                if (b.attack_timer <= 0) {
                    Id target = 0;
                    double best = b.kind == BuildingKind::Castle ? 7.5 : 8.0;
                    for (const auto &u : state_.units)
                        if (u.owner != b.owner && u.health > 0 && is_visible(b.owner, u.position) &&
                            !concealed(state_, u, b.owner) &&
                            distance(u.position, b.position) < best) {
                            best = distance(u.position, b.position);
                            target = u.id;
                        }
                    if (target && state_.projectiles.size() < MaximumEntities) {
                        state_.projectiles.push_back(
                            {state_.next_id++, b.owner, b.position, target,
                             (b.kind == BuildingKind::Castle ? 11.0 : 18.0) * b.level, 12, false});
                        b.attack_timer = 1.6;
                    }
                }
            }
        }
    for (const auto &r : recruits) {
        Id id = add_unit(r.owner, r.kind, r.position);
        auto *u = mutable_unit(id);
        u->order = OrderType::Move;
        order_path(*u, r.rally);
        event(r.owner, id, std::string(definition(r.kind).name) + " recruited.");
    }
}
void Simulation::damage(Id id, double amount, bool siege, Owner source) {
    Unit *u = mutable_unit(id);
    Building *b = mutable_building(id);
    if (!u && !b)
        return;
    Owner owner = u ? u->owner : b->owner;
    if (owner == source)
        return;
    Vec2 p = u ? u->position : b->position;
    for (const auto &e : state_.effects)
        if (e.owner == owner && e.kind == SpellKind::Shield && e.remaining > 0 &&
            distance(p, e.position) <= e.radius) {
            amount *= 0.4;
            break;
        }
    if (u) {
        double armor =
            definition(u->kind).armor + state_.factions[side(owner)].technologies[3] * 1.5;
        u->health -= std::max(1.0, amount - armor);
        if (u->health <= 0) {
            u->health = 0;
            if (u->illusion_seconds <= 0)
                ++state_.factions[side(owner)].units_lost;
        }
    } else {
        if (siege)
            amount *= 3.5;
        double armor =
            (b->kind == BuildingKind::Wall || b->kind == BuildingKind::Gate ? 8.0 : 2.0) +
            state_.factions[side(owner)].technologies[2] * 2.0;
        b->health -= std::max(1.0, amount - armor);
        b->last_damage_tick = state_.tick;
        if (b->health <= 0) {
            b->health = 0;
            ++state_.factions[side(owner)].buildings_lost;
            event(owner, b->id, std::string(definition(b->kind).name) + " destroyed.");
        }
    }
}
void Simulation::update_units() {
    auto hidden = [&](const Unit &target, Owner viewer) {
        return !is_visible(viewer, target.position) || concealed(state_, target, viewer);
    };
    for (auto &u : state_.units) {
        if (u.health <= 0)
            continue;
        const auto &d = definition(u.kind);
        auto &faction = state_.factions[side(u.owner)];
        u.attack_timer = std::max(0.0, u.attack_timer - FixedStep);
        u.repath_timer = std::max(0.0, u.repath_timer - FixedStep);
        for (auto &cd : u.spell_cooldowns)
            cd = std::max(0.0, cd - FixedStep);
        if (u.illusion_seconds > 0) {
            u.illusion_seconds -= FixedStep;
            if (u.illusion_seconds <= 0) {
                u.health = 0;
                continue;
            }
        }
        bool civilian = u.kind == UnitKind::Worker || u.kind == UnitKind::Engineer;
        // Idle military units defend their surroundings. Hold stance never pursues.
        if (!civilian &&
            (u.order == OrderType::Idle || u.order == OrderType::Patrol ||
             (u.order == OrderType::Move && u.stance == Stance::Aggressive)) &&
            u.stance != Stance::Hold) {
            Id target = 0;
            double best = u.stance == Stance::Aggressive ? 8.0 : 4.0;
            for (const auto &enemy : state_.units)
                if (enemy.owner != u.owner && enemy.health > 0 &&
                    is_visible(u.owner, enemy.position) && !hidden(enemy, u.owner) &&
                    distance(u.position, enemy.position) < best) {
                    best = distance(u.position, enemy.position);
                    target = enemy.id;
                }
            if (target) {
                u.order = OrderType::Attack;
                u.target_id = target;
                clear_path(u);
                u.repath_timer = 0;
            }
        }
        if (u.order == OrderType::Attack) {
            const Unit *target_u = unit(u.target_id);
            const Building *target_b = building(u.target_id);
            if ((!target_u && !target_b) || (target_u && hidden(*target_u, u.owner))) {
                u.target_id = 0;
                clear_path(u);
                if (u.patrolling) {
                    u.order = OrderType::Patrol;
                    order_path(u, u.patrol_destination);
                } else
                    u.order = OrderType::Idle;
                continue;
            }
            Vec2 target = target_u ? target_u->position : target_b->position;
            double range =
                target_u ? distance(u.position, target) : building_distance(*target_b, u.position);
            if (range <= d.range + 0.15) {
                clear_path(u);
                if (u.attack_timer <= 0 && u.illusion_seconds <= 0) {
                    double amount = d.damage * (0.75 + 0.25 * faction.morale / 100.0) *
                                    (1 + 0.12 * faction.technologies[3]);
                    if (u.kind == UnitKind::Spearman && target_u &&
                        target_u->kind == UnitKind::Cavalry)
                        amount *= 1.8;
                    if (d.range > 2) {
                        if (state_.projectiles.size() < MaximumEntities)
                            state_.projectiles.push_back(
                                {state_.next_id++, u.owner, u.position, u.target_id, amount,
                                 u.kind == UnitKind::Catapult ? 7.0 : 12.0, d.siege});
                    } else
                        damage(u.target_id, amount, d.siege, u.owner);
                    u.attack_timer = d.cooldown;
                }
            } else if (u.stance == Stance::Hold)
                clear_path(u);
            else if (u.repath_timer <= 0 || distance(u.destination, target) > 1.5)
                order_path(u, target);
        }
        if (u.order == OrderType::ReturnCargo) {
            const Building *depot = nullptr;
            double best = 1e30;
            for (const auto &b : state_.buildings)
                if (b.owner == u.owner && complete(b) && storage(b.kind) &&
                    distance(u.position, b.position) < best) {
                    best = distance(u.position, b.position);
                    depot = &b;
                }
            if (!depot) {
                clear_path(u);
                continue;
            }
            if (building_distance(*depot, u.position) <= 1.1) {
                double available = std::max(0.0, faction.capacity.get(u.cargo_kind) -
                                                     faction.resources.get(u.cargo_kind));
                double delivered = std::min(u.cargo, available);
                deposit(faction, u.cargo_kind, delivered);
                u.cargo -= delivered;
                if (u.automatic_work && available <= 1e-8) {
                    u.cargo = 0;
                    u.order = OrderType::Idle;
                    u.job_id = 0;
                    u.target_id = 0;
                    clear_path(u);
                }
                if (u.cargo <= 1e-8) {
                    u.cargo = 0;
                    u.order = OrderType::Gather;
                    clear_path(u);
                    u.repath_timer = 0;
                }
            } else if (u.path.empty() || u.repath_timer <= 0)
                order_path(u, depot->position);
        }
        if (u.order == OrderType::Gather) {
            auto *n = mutable_node(u.target_id);
            auto *farm = mutable_building(u.target_id);
            bool farming = farm && farm->owner == u.owner && farm->kind == BuildingKind::Farm &&
                           complete(*farm);
            if ((!n || n->remaining <= 0) && !farming) {
                u.order = OrderType::Idle;
                u.target_id = 0;
                u.job_id = 0;
                clear_path(u);
                continue;
            }
            Vec2 target = farming ? farm->position : n->position;
            double range =
                farming ? building_distance(*farm, u.position) : distance(u.position, target);
            if (range <= 1.15) {
                clear_path(u);
                ResourceKind kind = farming ? ResourceKind::Food : n->kind;
                double rate = (farming ? 3.0 : 2.4) * (0.6 + 0.4 * faction.morale / 100.0);
                if (farming) {
                    rate *= 1 + 0.2 * faction.technologies[1];
                    rate *= farm->level;
                    bool mill = false;
                    for (const auto &b : state_.buildings)
                        if (b.owner == u.owner && b.kind == BuildingKind::Mill && complete(b) &&
                            distance(b.position, farm->position) <= 12)
                            mill = true;
                    if (mill && faction.resources.wood >= 0.03) {
                        faction.resources.wood -= 0.03;
                        rate *= 1.4;
                    }
                    if (farm->connected)
                        rate *= 1.2;
                    for (const auto &e : state_.effects)
                        if (e.owner == u.owner && e.kind == SpellKind::HarvestBloom &&
                            e.remaining > 0 && distance(e.position, farm->position) <= e.radius) {
                            rate *= 2;
                            break;
                        }
                } else {
                    if (kind == ResourceKind::Wood)
                        rate *= 1 + 0.2 * faction.technologies[0];
                    if (auto *job = building(u.job_id))
                        rate *= 1.35 * job->level;
                }
                if (u.cargo > 0 && u.cargo_kind != kind) {
                    u.order = OrderType::ReturnCargo;
                    continue;
                }
                double amount = std::min(12.0 - u.cargo, rate * FixedStep);
                if (!farming) {
                    amount = std::min(amount, n->remaining);
                    n->remaining -= amount;
                }
                u.cargo_kind = kind;
                u.cargo += amount;
                if (farming && farm->level < 3 && amount > 0)
                    farm->harvested_food = std::min(farm_milestone(*farm), farm->harvested_food + amount);
                if (u.cargo >= 12 - 1e-8 || (!farming && n->remaining <= 0)) {
                    u.order = OrderType::ReturnCargo;
                    clear_path(u);
                    u.repath_timer = 0;
                }
            } else if (u.path.empty() || u.repath_timer <= 0)
                order_path(u, target);
        }
        if (u.order == OrderType::Build || u.order == OrderType::Repair) {
            auto *b = mutable_building(u.target_id);
            if (!b || b->owner != u.owner) {
                u.order = OrderType::Idle;
                u.target_id = 0;
                clear_path(u);
                continue;
            }
            if (building_distance(*b, u.position) <= 1.15) {
                clear_path(u);
                double rate = u.kind == UnitKind::Engineer ? 1.75 : 1.0;
                if (b->progress < 1) {
                    b->progress = std::min(
                        1.0, b->progress + FixedStep * rate / definition(b->kind).build_seconds);
                    if (b->progress >= 1) {
                        event(b->owner, b->id,
                              std::string(definition(b->kind).name) + " completed.");
                        update_network();
                    }
                } else if (u.order == OrderType::Repair && b->health < max_health(*b)) {
                    Resources cost = resources(0.025 * rate, 0.02 * rate);
                    if (faction.resources.can_afford(cost)) {
                        subtract(faction.resources, cost);
                        b->health = std::min(max_health(*b), b->health + 2.2 * rate);
                    }
                } else {
                    u.order = OrderType::Idle;
                    u.target_id = 0;
                    clear_path(u);
                }
            } else if (u.path.empty() || u.repath_timer <= 0)
                order_path(u, b->position);
        }
        if ((u.order == OrderType::Move || u.order == OrderType::Patrol) && u.path.empty() &&
            u.repath_timer <= 0)
            order_path(u, u.destination);
        if (u.path_index < u.path.size()) {
            double budget = d.speed * FixedStep * (0.6 + 0.4 * faction.morale / 100.0);
            while (budget > 0 && u.path_index < u.path.size()) {
                Vec2 next = u.path[u.path_index];
                if (!walkable(next, u.owner)) {
                    clear_path(u);
                    u.path_index = 0;
                    u.repath_timer = 0;
                    break;
                }
                double delta = distance(u.position, next);
                if (delta <= budget + 1e-8) {
                    u.position = next;
                    budget -= delta;
                    ++u.path_index;
                } else {
                    u.position.x += (next.x - u.position.x) * budget / delta;
                    u.position.y += (next.y - u.position.y) * budget / delta;
                    budget = 0;
                }
            }
        }
        if (u.path_index >= u.path.size() && !u.path.empty()) {
            clear_path(u);
            u.path_index = 0;
            if (u.order == OrderType::Move)
                u.order = OrderType::Idle;
            else if (u.order == OrderType::Patrol) {
                Vec2 next = distance(u.destination, u.patrol_destination) < 0.5
                                ? u.patrol_origin
                                : u.patrol_destination;
                order_path(u, next);
            }
        }
    }
}
void Simulation::update_projectiles() {
    for (auto &p : state_.projectiles) {
        const Unit *u = unit(p.target_id);
        const Building *b = building(p.target_id);
        if (!u && !b) {
            p.target_id = 0;
            continue;
        }
        Vec2 target = u ? u->position : b->position;
        double delta = distance(p.position, target);
        if (delta <= p.speed * FixedStep + 0.4) {
            damage(p.target_id, p.damage, p.siege, p.owner);
            p.target_id = 0;
        } else {
            p.position.x += (target.x - p.position.x) * p.speed * FixedStep / delta;
            p.position.y += (target.y - p.position.y) * p.speed * FixedStep / delta;
        }
    }
    state_.projectiles.erase(std::remove_if(state_.projectiles.begin(), state_.projectiles.end(),
                                            [](const auto &p) { return p.target_id == 0; }),
                             state_.projectiles.end());
}
void Simulation::update_effects() {
    for (auto &e : state_.effects)
        e.remaining = std::max(0.0, e.remaining - FixedStep);
    // A closing bridge displaces stranded units to their bank. It never leaves
    // units permanently trapped on a water cell or silently kills their army.
    for (const auto &e : state_.effects)
        if (e.remaining <= 0 && e.kind == SpellKind::LightBridge)
            for (auto &u : state_.units)
                if (u.health > 0 && distance(u.position, e.position) <= e.radius &&
                    !walkable(u.position, u.owner)) {
                    Vec2 best{};
                    double score = 1e30;
                    for (int y = std::max(1, static_cast<int>(u.position.y) - 8);
                         y <= std::min(state_.height - 2, static_cast<int>(u.position.y) + 8); ++y)
                        for (int x = std::max(1, static_cast<int>(u.position.x) - 8);
                             x <= std::min(state_.width - 2, static_cast<int>(u.position.x) + 8);
                             ++x) {
                            Vec2 p{x + 0.5, y + 0.5};
                            double d = distance(u.position, p);
                            if (walkable(p, u.owner) && d < score) {
                                best = p;
                                score = d;
                            }
                        }
                    if (score < 1e30) {
                        u.position = best;
                        clear_path(u);
                        u.repath_timer = 0;
                    }
                }
    state_.effects.erase(std::remove_if(state_.effects.begin(), state_.effects.end(),
                                        [](const auto &e) { return e.remaining <= 0; }),
                         state_.effects.end());
}
void Simulation::update_visibility() {
    std::size_t size = static_cast<std::size_t>(state_.width * state_.height);
    for (int s = 0; s < 2; ++s) {
        state_.visible[s].assign(size, 0);
        if (state_.explored[s].size() != size)
            state_.explored[s].assign(size, 0);
    }
    auto reveal = [&](Owner owner, Vec2 p, double radius) {
        if (owner == Owner::Neutral)
            return;
        int s = side(owner), loX = std::max(0, static_cast<int>(p.x - radius)),
            hiX = std::min(state_.width - 1, static_cast<int>(p.x + radius));
        int loY = std::max(0, static_cast<int>(p.y - radius)),
            hiY = std::min(state_.height - 1, static_cast<int>(p.y + radius));
        for (int y = loY; y <= hiY; ++y)
            for (int x = loX; x <= hiX; ++x)
                if (distance({x + 0.5, y + 0.5}, p) <= radius) {
                    auto index = static_cast<std::size_t>(y * state_.width + x);
                    state_.visible[s][index] = 1;
                    state_.explored[s][index] = 1;
                }
    };
    for (const auto &b : state_.buildings)
        if (b.health > 0)
            reveal(b.owner, b.position,
                   complete(b) ? b.kind == BuildingKind::Castle   ? 13
                                 : b.kind == BuildingKind::Beacon ? b.connected ? 13 : 8
                                                                  : 8
                               : 4);
    for (const auto &u : state_.units)
        if (u.health > 0)
            reveal(u.owner, u.position, u.kind == UnitKind::Wizard ? 11 : 8);
}
void Simulation::update_outcome() {
    if (state_.outcome != MatchOutcome::Playing)
        return;
    for (int s = 0; s < 2; ++s) {
        bool castle = false;
        Owner owner = s ? Owner::Enemy : Owner::Player;
        for (const auto &b : state_.buildings)
            if (b.owner == owner && b.kind == BuildingKind::Castle && b.health > 0)
                castle = true;
        if (!castle) {
            state_.outcome = s ? MatchOutcome::PlayerVictory : MatchOutcome::EnemyVictory;
            event(owner, 0, "The Dawnkeep has fallen.");
            return;
        }
        const auto &map = map_catalog()[state_.map_index];
        if (map.objective_beacons) {
            int occupied = 0;
            for (auto site : state_.objective_sites)
                for (const auto &b : state_.buildings)
                    if (b.owner == owner && b.kind == BuildingKind::Beacon && complete(b) &&
                        b.connected && distance(b.position, site) < 2.5) {
                        ++occupied;
                        break;
                    }
            if (occupied >= map.objective_beacons)
                state_.factions[s].objective_hold += FixedStep;
            else
                state_.factions[s].objective_hold = 0;
            if (state_.factions[s].objective_hold + 1e-8 >= map.objective_seconds) {
                state_.outcome = s ? MatchOutcome::EnemyVictory : MatchOutcome::PlayerVictory;
                event(owner, 0, "The ancient lanterns awaken. The realm is yours.");
                return;
            }
        }
    }
}

void Simulation::update_ai() {
    state_.ai_timer -= FixedStep;
    if (state_.ai_timer > 0)
        return;
    state_.ai_timer = state_.difficulty == Difficulty::Easy   ? 5.0
                      : state_.difficulty == Difficulty::Hard ? 1.5
                                                              : 2.8;
    Owner owner = Owner::Enemy;
    const auto &map = map_catalog()[state_.map_index];
    Vec2 base = map.enemy_start;
    ++state_.ai_phase;
    auto count_building = [&](BuildingKind kind, bool finished = false) {
        int n = 0;
        for (const auto &b : state_.buildings)
            if (b.owner == owner && b.health > 0 && b.kind == kind && (!finished || complete(b)))
                ++n;
        return n;
    };
    auto first_building = [&](BuildingKind kind) {
        for (const auto &b : state_.buildings)
            if (b.owner == owner && b.kind == kind && complete(b))
                return b.id;
        return Id{0};
    };
    auto construct = [&](BuildingKind kind, Vec2 near, bool forward = false) {
        Command c;
        c.owner = owner;
        c.type = CommandType::Build;
        c.building = kind;
        // The same validator and costs govern both factions; no hidden resource grants.
        for (int ring = 0; ring < 12; ++ring)
            for (int n = 0; n < 12; ++n) {
                double angle = (n + (state_.ai_phase % 12) * 0.25) * Pi / 6;
                double r = ring ? 3.0 + ring : 0;
                c.position = centre({near.x + std::cos(angle) * r, near.y + std::sin(angle) * r});
                if (!forward && distance(c.position, base) < 4)
                    continue;
                if (can_build(owner, kind, c.position) && issue(c))
                    return true;
            }
        return false;
    };
    auto recruit = [&](UnitKind kind) {
        Id id = first_building(definition(kind).recruited_at);
        if (!id)
            return false;
        auto *b = building(id);
        if (!b || b->train_queue.size() >= 2)
            return false;
        Command c;
        c.owner = owner;
        c.type = CommandType::Train;
        c.target_id = id;
        c.unit = kind;
        return static_cast<bool>(issue(c));
    };
    int workers = 0, army = 0;
    for (const auto &u : state_.units)
        if (u.owner == owner && u.health > 0 && u.illusion_seconds <= 0) {
            if (u.kind == UnitKind::Worker)
                ++workers;
            else if (u.kind != UnitKind::Engineer && u.kind != UnitKind::Wizard)
                ++army;
        }
    const auto &faction = state_.factions[1];
    if (count_building(BuildingKind::Farm) == 0)
        construct(BuildingKind::Farm, {base.x - 6, base.y + 2});
    else if (faction.population_limit - faction.population < 4)
        construct(BuildingKind::House, {base.x + 5, base.y + 1});
    else if (count_building(BuildingKind::LumberCamp) == 0)
        construct(BuildingKind::LumberCamp, {base.x - 7, base.y - 6});
    else if (count_building(BuildingKind::Barracks) == 0)
        construct(BuildingKind::Barracks, {base.x - 6, base.y - 3});
    else if (count_building(BuildingKind::Mill) == 0)
        construct(BuildingKind::Mill, {base.x - 2, base.y + 7});
    else if (count_building(BuildingKind::Quarry) == 0)
        construct(BuildingKind::Quarry, {base.x - 9, base.y + 7});
    else if (count_building(BuildingKind::GoldMine) == 0)
        construct(BuildingKind::GoldMine, {base.x + 5, base.y - 7});
    else if (count_building(BuildingKind::Farm) < 2 && faction.resources.food < 120)
        construct(BuildingKind::Farm, {base.x - 5, base.y + 7});
    else if (state_.ai_personality == AIPersonality::Arcanist &&
             count_building(BuildingKind::MageTower) == 0)
        construct(BuildingKind::MageTower, {base.x - 4, base.y - 6});
    else if (count_building(BuildingKind::ArcherRange) == 0)
        construct(BuildingKind::ArcherRange, {base.x - 6, base.y + 6});
    else if (count_building(BuildingKind::Forge) == 0)
        construct(BuildingKind::Forge, {base.x + 4, base.y + 6});
    else if (count_building(BuildingKind::SiegeWorkshop) == 0 && state_.tick > 900)
        construct(BuildingKind::SiegeWorkshop, {base.x + 4, base.y - 4});
    else if (state_.ai_personality == AIPersonality::Builder &&
             count_building(BuildingKind::Watchtower) < 2)
        construct(BuildingKind::Watchtower,
                  {base.x - 9, base.y + count_building(BuildingKind::Watchtower) * 7 - 3});
    else if (count_building(BuildingKind::Beacon) < 5) {
        Vec2 forward{base.x - 11, base.y};
        double best = forward.x;
        for (const auto &b : state_.buildings)
            if (b.owner == owner && complete(b) && b.kind == BuildingKind::Beacon && b.connected &&
                b.position.x < best + 11) {
                best = b.position.x - 10;
                forward = {best, b.position.y};
            }
        if (forward.x > 15)
            construct(BuildingKind::Beacon, forward, true);
    }
    if (workers < 12 && state_.ai_phase % 3 == 0)
        recruit(UnitKind::Worker);
    int desired = state_.ai_personality == AIPersonality::Conqueror ? 16
                  : state_.ai_personality == AIPersonality::Builder ? 11
                                                                    : 13;
    if (army < desired || state_.tick > 3000) {
        UnitKind kind = UnitKind::Militia;
        if (state_.ai_phase % 6 == 0 && first_building(BuildingKind::SiegeWorkshop))
            kind = UnitKind::Catapult;
        else if (state_.ai_phase % 7 == 0 && first_building(BuildingKind::SiegeWorkshop))
            kind = UnitKind::Ram;
        else if (state_.ai_personality == AIPersonality::Arcanist && state_.ai_phase % 3 == 0 &&
                 first_building(BuildingKind::MageTower))
            kind = UnitKind::ArcaneGuard;
        else if (state_.ai_phase % 3 == 0 && first_building(BuildingKind::ArcherRange))
            kind = UnitKind::Archer;
        else if (state_.ai_phase % 4 == 0 && first_building(BuildingKind::Forge))
            kind = UnitKind::Swordsman;
        else if (state_.ai_phase % 2 == 0)
            kind = UnitKind::Spearman;
        recruit(kind);
    }
    if (state_.ai_phase % 16 == 0) {
        Command c;
        c.owner = owner;
        c.type = CommandType::Research;
        c.technology = state_.ai_personality == AIPersonality::Arcanist  ? Technology::Arcana
                       : state_.ai_personality == AIPersonality::Builder ? Technology::Masonry
                                                                         : Technology::Steel;
        issue(c);
    }
    // Early scout uses actual navigation and sight; enemy buildings become
    // attack targets only after they are revealed by a unit or a light node.
    if (state_.tick > 300 && state_.tick < 1200 && state_.ai_phase % 6 == 0) {
        for (const auto &u : state_.units)
            if (u.owner == owner && u.kind == UnitKind::Militia && u.health > 0 &&
                u.order == OrderType::Idle) {
                Command c;
                c.owner = owner;
                c.type = CommandType::Move;
                c.ids = {u.id};
                c.position = {map.player_start.x + 6, map.player_start.y - 4};
                issue(c);
                break;
            }
    }
    double attack_time = state_.ai_personality == AIPersonality::Conqueror ? 110
                         : state_.ai_personality == AIPersonality::Builder ? 190
                                                                           : 150;
    if (state_.difficulty == Difficulty::Easy)
        attack_time *= 1.5;
    if (state_.difficulty == Difficulty::Hard)
        attack_time *= 0.8;
    if (state_.tick * FixedStep > attack_time && state_.ai_phase % 5 == 0) {
        Id target = 0;
        double priority = 1e30;
        for (const auto &b : state_.buildings)
            if (b.owner == Owner::Player && b.health > 0 && is_visible(owner, b.position)) {
                double score = distance(base, b.position) - (b.kind == BuildingKind::Beacon   ? 14
                                                             : b.kind == BuildingKind::Castle ? 6
                                                                                              : 0);
                if (score < priority) {
                    target = b.id;
                    priority = score;
                }
            }
        Command c;
        c.owner = owner;
        c.type = target ? CommandType::Attack : CommandType::Move;
        c.target_id = target;
        c.position = map.player_start;
        int guards = state_.ai_personality == AIPersonality::Builder ? 2 : 0;
        for (const auto &u : state_.units)
            if (u.owner == owner && u.health > 0 && u.kind != UnitKind::Worker &&
                u.kind != UnitKind::Engineer && u.kind != UnitKind::Wizard &&
                u.illusion_seconds <= 0) {
                if (guards-- > 0)
                    continue;
                if (u.order != OrderType::Attack)
                    c.ids.push_back(u.id);
            }
        if (!c.ids.empty())
            issue(c);
        if (state_.ai_personality == AIPersonality::Arcanist)
            for (const auto &u : state_.units)
                if (u.owner == owner && u.kind == UnitKind::Wizard && u.health > 0) {
                    Command move;
                    move.owner = owner;
                    move.type = CommandType::Move;
                    move.ids = {u.id};
                    move.position = {std::max(map.player_start.x + 10, base.x - 18),
                                     map.player_start.y};
                    issue(move);
                    break;
                }
    }
    // Spell users obey the same Weaver range, sight, mana and cooldown rules.
    std::vector<Id> wizards;
    for (const auto &u : state_.units)
        if (u.owner == owner && u.kind == UnitKind::Wizard && u.health > 0)
            wizards.push_back(u.id);
    for (auto id : wizards) {
        const auto *wizard = unit(id);
        if (!wizard)
            continue;
        Command c;
        c.owner = owner;
        c.type = CommandType::CastSpell;
        c.ids = {id};
        c.position = wizard->position;
        bool injury = false, threat = false;
        for (const auto &u : state_.units)
            if (u.health > 0 && distance(u.position, wizard->position) < 5) {
                if (u.owner == owner && u.health < max_health(u) * 0.65)
                    injury = true;
                if (u.owner == Owner::Player)
                    threat = true;
            }
        if (injury) {
            c.spell = SpellKind::Heal;
            issue(c);
        }
        if (threat) {
            c.spell = SpellKind::Shield;
            issue(c);
        }
        if (state_.ai_personality == AIPersonality::Arcanist)
            for (const auto &b : state_.buildings)
                if (b.owner == Owner::Player && b.kind == BuildingKind::Beacon && complete(b) &&
                    distance(b.position, wizard->position) <
                        definition(SpellKind::Disrupt).cast_range) {
                    c.spell = SpellKind::Disrupt;
                    c.position = b.position;
                    c.target_id = b.id;
                    issue(c);
                    break;
                }
    }
}
void Simulation::fixed_step() {
    ++state_.tick;
    for (auto &faction : state_.factions)
        if (faction.automatic_growth_cooldown_ticks)
            --faction.automatic_growth_cooldown_ticks;
    update_effects();
    update_network();
    recalculate_factions();
    update_economy();
    if (state_.tick % 10 == 1)
        assign_jobs();
    update_units();
    update_projectiles();
    update_ai();
    state_.units.erase(std::remove_if(state_.units.begin(), state_.units.end(),
                                      [](const auto &u) { return u.health <= 0; }),
                       state_.units.end());
    state_.buildings.erase(std::remove_if(state_.buildings.begin(), state_.buildings.end(),
                                          [](const auto &b) { return b.health <= 0; }),
                           state_.buildings.end());
    recalculate_factions();
    update_network();
    update_visibility();
    update_outcome();
    update_automatic_growth();
}
void Simulation::step(double seconds) {
    if (!std::isfinite(seconds) || seconds <= 0 || seconds > 3600 ||
        state_.outcome != MatchOutcome::Playing)
        return;
    state_.accumulator += seconds;
    while (state_.accumulator + 1e-9 >= FixedStep && state_.outcome == MatchOutcome::Playing) {
        state_.accumulator = std::max(0.0, state_.accumulator - FixedStep);
        fixed_step();
    }
    if (state_.outcome != MatchOutcome::Playing || state_.accumulator < 1e-9)
        state_.accumulator = 0;
}

namespace {
// Explicit little-endian binary payload wrapped in hexadecimal ASCII. No native
// struct layouts, locale parsing, exceptions, or unsafe unbounded allocations.
struct Writer {
    std::string bytes;
    void u8(std::uint8_t v) { bytes.push_back(static_cast<char>(v)); }
    void u32(std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            u8(static_cast<std::uint8_t>(v >> (i * 8)));
    }
    void u64(std::uint64_t v) {
        for (int i = 0; i < 8; ++i)
            u8(static_cast<std::uint8_t>(v >> (i * 8)));
    }
    void number(double v) { u64(std::bit_cast<std::uint64_t>(v)); }
    void point(Vec2 v) {
        number(v.x);
        number(v.y);
    }
    template <class E> void enumeration(E v) { u8(static_cast<std::uint8_t>(v)); }
    void resource(const Resources &v) {
        number(v.wood);
        number(v.stone);
        number(v.food);
        number(v.gold);
        number(v.mana);
    }
    void text(const std::string &v) {
        u32(static_cast<std::uint32_t>(v.size()));
        bytes.append(v);
    }
    void points(const std::vector<Vec2> &v) {
        u32(static_cast<std::uint32_t>(v.size()));
        for (auto p : v)
            point(p);
    }
};
struct Reader {
    const std::string &bytes;
    std::size_t index = 0;
    bool ok = true;
    std::uint8_t u8() {
        if (index >= bytes.size()) {
            ok = false;
            return 0;
        }
        return static_cast<std::uint8_t>(bytes[index++]);
    }
    std::uint32_t u32() {
        std::uint32_t v = 0;
        for (int i = 0; i < 4; ++i)
            v |= static_cast<std::uint32_t>(u8()) << (i * 8);
        return v;
    }
    std::uint64_t u64() {
        std::uint64_t v = 0;
        for (int i = 0; i < 8; ++i)
            v |= static_cast<std::uint64_t>(u8()) << (i * 8);
        return v;
    }
    double number() { return std::bit_cast<double>(u64()); }
    Vec2 point() { return {number(), number()}; }
    template <class E> E enumeration() { return static_cast<E>(u8()); }
    Resources resource() { return {number(), number(), number(), number(), number()}; }
    std::size_t count(std::size_t limit) {
        auto n = u32();
        if (n > limit) {
            ok = false;
            return 0;
        }
        return n;
    }
    std::string text(std::size_t limit) {
        auto n = count(limit);
        if (n > bytes.size() - std::min(index, bytes.size())) {
            ok = false;
            return {};
        };
        auto v = bytes.substr(index, n);
        index += n;
        return v;
    }
    std::vector<Vec2> points(std::size_t limit) {
        auto n = count(limit);
        std::vector<Vec2> v;
        v.reserve(n);
        for (std::size_t i = 0; i < n && ok; ++i)
            v.push_back(point());
        return v;
    }
};
std::uint32_t checksum(const std::string &data) {
    std::uint32_t v = 2166136261u;
    for (unsigned char c : data) {
        v ^= c;
        v *= 16777619u;
    }
    return v;
}
int nibble(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}
} // namespace
std::string Simulation::serialize() const {
    const auto &s = state_;
    Writer w;
    w.u64(s.tick);
    w.number(s.accumulator);
    w.u32(s.rng);
    w.u32(s.next_id);
    w.u32(s.map_index);
    w.u32(static_cast<std::uint32_t>(s.width));
    w.u32(static_cast<std::uint32_t>(s.height));
    w.enumeration(s.ai_personality);
    w.enumeration(s.difficulty);
    w.enumeration(s.outcome);
    w.u32(static_cast<std::uint32_t>(s.terrain.size()));
    for (auto t : s.terrain)
        w.enumeration(t);
    w.points(s.crossing_sites);
    w.points(s.objective_sites);
    for (int f = 0; f < 2; ++f)
        for (const auto *mask : {&s.explored[f], &s.visible[f]}) {
            w.u32(static_cast<std::uint32_t>(mask->size()));
            for (auto v : *mask)
                w.u8(v);
        }
    for (const auto &f : s.factions) {
        w.resource(f.resources);
        w.resource(f.capacity);
        w.u32(static_cast<std::uint32_t>(f.population));
        w.u32(static_cast<std::uint32_t>(f.population_limit));
        w.number(f.morale);
        w.number(f.objective_hold);
        for (auto t : f.technologies)
            w.u8(t);
        w.enumeration(f.research);
        w.number(f.research_timer);
        w.u32(f.units_lost);
        w.u32(f.units_recruited);
        w.u32(f.buildings_lost);
    }
    w.u32(static_cast<std::uint32_t>(s.buildings.size()));
    for (const auto &b : s.buildings) {
        w.u32(b.id);
        w.enumeration(b.owner);
        w.enumeration(b.kind);
        w.point(b.position);
        w.u8(b.rotation);
        w.number(b.health);
        w.number(b.progress);
        w.u8(b.level);
        w.u8(b.connected ? 1 : 0);
        w.number(b.production_timer);
        w.number(b.train_timer);
        w.number(b.attack_timer);
        w.point(b.rally);
        w.u32(static_cast<std::uint32_t>(b.train_queue.size()));
        for (auto kind : b.train_queue)
            w.enumeration(kind);
    }
    w.u32(static_cast<std::uint32_t>(s.units.size()));
    for (const auto &u : s.units) {
        w.u32(u.id);
        w.enumeration(u.owner);
        w.enumeration(u.kind);
        w.point(u.position);
        w.point(u.destination);
        w.point(u.patrol_origin);
        w.number(u.health);
        w.number(u.attack_timer);
        w.number(u.work_timer);
        w.enumeration(u.order);
        w.enumeration(u.stance);
        w.u32(u.target_id);
        w.u32(u.job_id);
        w.enumeration(u.cargo_kind);
        w.number(u.cargo);
        w.points(u.path);
        w.u32(u.path_index);
        w.number(u.repath_timer);
        for (auto cd : u.spell_cooldowns)
            w.number(cd);
        w.number(u.illusion_seconds);
        w.u8(u.automatic_work ? 1 : 0);
        w.u8(u.patrolling ? 1 : 0);
        w.point(u.patrol_destination);
    }
    w.u32(static_cast<std::uint32_t>(s.nodes.size()));
    for (const auto &n : s.nodes) {
        w.u32(n.id);
        w.enumeration(n.kind);
        w.point(n.position);
        w.number(n.remaining);
    }
    w.u32(static_cast<std::uint32_t>(s.effects.size()));
    for (const auto &e : s.effects) {
        w.u32(e.id);
        w.enumeration(e.owner);
        w.enumeration(e.kind);
        w.point(e.position);
        w.number(e.remaining);
        w.number(e.radius);
        w.u32(e.target_id);
    }
    w.u32(static_cast<std::uint32_t>(s.projectiles.size()));
    for (const auto &p : s.projectiles) {
        w.u32(p.id);
        w.enumeration(p.owner);
        w.point(p.position);
        w.u32(p.target_id);
        w.number(p.damage);
        w.number(p.speed);
        w.u8(p.siege ? 1 : 0);
    }
    w.u32(static_cast<std::uint32_t>(s.events.size()));
    for (const auto &e : s.events) {
        w.u64(e.tick);
        w.enumeration(e.owner);
        w.u32(e.subject_id);
        w.text(e.text);
    }
    w.number(s.ai_timer);
    w.number(s.upkeep_timer);
    w.u32(s.ai_phase);
    // Preserve the complete v1 payload above; all new state is in this extension.
    w.u32(static_cast<std::uint32_t>(s.factions.size()));
    for (const auto &faction : s.factions) {
        w.u8(faction.automatic_growth.enabled ? 1 : 0);
        w.number(faction.automatic_growth.protected_wood);
        w.number(faction.automatic_growth.protected_stone);
        w.u32(faction.housing_demand_ticks);
        w.u32(faction.automatic_growth_cooldown_ticks);
    }
    w.u32(static_cast<std::uint32_t>(s.buildings.size()));
    for (const auto &building : s.buildings) {
        w.u32(building.id);
        w.number(building.harvested_food);
        w.u64(building.last_damage_tick);
    }
    std::string result = "WILDLIGHT " + std::to_string(SaveVersion) + " " +
                         std::to_string(checksum(w.bytes)) + "\n";
    result.reserve(result.size() + w.bytes.size() * 2);
    constexpr char hex[] = "0123456789abcdef";
    for (unsigned char c : w.bytes) {
        result.push_back(hex[c >> 4]);
        result.push_back(hex[c & 15]);
    }
    return result;
}
LoadResult Simulation::deserialize(const std::string &input) {
    auto fail = [](const char *reason) { return LoadResult{false, reason}; };
    if (input.size() > MaximumSaveBytes)
        return fail("Save exceeds the 8 MiB safety limit.");
    auto newline = input.find('\n');
    if (newline == std::string::npos || newline > 64)
        return fail("Invalid save header.");
    constexpr std::string_view legacy_prefix = "WILDLIGHT 1 ", current_prefix = "WILDLIGHT 2 ";
    const bool legacy = input.compare(0, legacy_prefix.size(), legacy_prefix) == 0;
    if (!legacy && input.compare(0, current_prefix.size(), current_prefix) != 0)
        return fail("Unsupported save format or schema version.");
    std::uint32_t expected = 0;
    auto parsed = std::from_chars(input.data() + current_prefix.size(), input.data() + newline, expected);
    if (parsed.ec != std::errc{} || parsed.ptr != input.data() + newline)
        return fail("Invalid save checksum field.");
    if ((input.size() - newline - 1) % 2)
        return fail("Truncated hexadecimal payload.");
    std::string raw;
    raw.reserve((input.size() - newline - 1) / 2);
    for (std::size_t i = newline + 1; i < input.size(); i += 2) {
        int a = nibble(input[i]), b = nibble(input[i + 1]);
        if (a < 0 || b < 0)
            return fail("Invalid hexadecimal payload.");
        raw.push_back(static_cast<char>(a * 16 + b));
    }
    if (checksum(raw) != expected)
        return fail("Save checksum mismatch; the match was not changed.");
    Reader r{raw};
    State s;
    s.tick = r.u64();
    s.accumulator = r.number();
    s.rng = r.u32();
    s.next_id = r.u32();
    s.map_index = r.u32();
    auto width = r.u32(), height = r.u32();
    if (width != 64 || height != 48 || s.map_index >= 3)
        return fail("Invalid map dimensions or index.");
    s.width = static_cast<int>(width);
    s.height = static_cast<int>(height);
    s.ai_personality = r.enumeration<AIPersonality>();
    s.difficulty = r.enumeration<Difficulty>();
    s.outcome = r.enumeration<MatchOutcome>();
    auto terrain_count = r.count(64 * 48);
    for (std::size_t i = 0; i < terrain_count && r.ok; ++i)
        s.terrain.push_back(r.enumeration<Terrain>());
    s.crossing_sites = r.points(16);
    s.objective_sites = r.points(16);
    for (int f = 0; f < 2; ++f)
        for (auto *mask : {&s.explored[f], &s.visible[f]}) {
            auto count = r.count(64 * 48);
            for (std::size_t i = 0; i < count && r.ok; ++i)
                mask->push_back(r.u8());
        }
    for (auto &f : s.factions) {
        f.resources = r.resource();
        f.capacity = r.resource();
        auto population = r.u32(), limit = r.u32();
        if (population > MaximumEntities * 4 || limit > MaximumEntities * 64)
            return fail("Invalid population data.");
        f.population = static_cast<int>(population);
        f.population_limit = static_cast<int>(limit);
        f.morale = r.number();
        f.objective_hold = r.number();
        for (auto &t : f.technologies)
            t = r.u8();
        f.research = r.enumeration<Technology>();
        f.research_timer = r.number();
        f.units_lost = r.u32();
        f.units_recruited = r.u32();
        f.buildings_lost = r.u32();
    }
    auto count = r.count(MaximumEntities);
    for (std::size_t i = 0; i < count && r.ok; ++i) {
        Building b;
        b.id = r.u32();
        b.owner = r.enumeration<Owner>();
        b.kind = r.enumeration<BuildingKind>();
        b.position = r.point();
        b.rotation = r.u8();
        b.health = r.number();
        b.progress = r.number();
        b.level = r.u8();
        auto connected = r.u8();
        if (connected > 1)
            r.ok = false;
        b.connected = connected != 0;
        b.production_timer = r.number();
        b.train_timer = r.number();
        b.attack_timer = r.number();
        b.rally = r.point();
        auto queue = r.count(8);
        for (std::size_t n = 0; n < queue && r.ok; ++n)
            b.train_queue.push_back(r.enumeration<UnitKind>());
        s.buildings.push_back(std::move(b));
    }
    count = r.count(MaximumEntities);
    for (std::size_t i = 0; i < count && r.ok; ++i) {
        Unit u;
        u.id = r.u32();
        u.owner = r.enumeration<Owner>();
        u.kind = r.enumeration<UnitKind>();
        u.position = r.point();
        u.destination = r.point();
        u.patrol_origin = r.point();
        u.health = r.number();
        u.attack_timer = r.number();
        u.work_timer = r.number();
        u.order = r.enumeration<OrderType>();
        u.stance = r.enumeration<Stance>();
        u.target_id = r.u32();
        u.job_id = r.u32();
        u.cargo_kind = r.enumeration<ResourceKind>();
        u.cargo = r.number();
        u.path = r.points(64 * 48);
        u.path_index = r.u32();
        u.repath_timer = r.number();
        for (auto &cd : u.spell_cooldowns)
            cd = r.number();
        u.illusion_seconds = r.number();
        auto automatic = r.u8();
        if (automatic > 1)
            r.ok = false;
        u.automatic_work = automatic != 0;
        auto patrolling = r.u8();
        if (patrolling > 1)
            r.ok = false;
        u.patrolling = patrolling != 0;
        u.patrol_destination = r.point();
        s.units.push_back(std::move(u));
    }
    count = r.count(MaximumEntities);
    for (std::size_t i = 0; i < count && r.ok; ++i) {
        ResourceNode n;
        n.id = r.u32();
        n.kind = r.enumeration<ResourceKind>();
        n.position = r.point();
        n.remaining = r.number();
        s.nodes.push_back(n);
    }
    count = r.count(MaximumEntities);
    for (std::size_t i = 0; i < count && r.ok; ++i) {
        SpellEffect e;
        e.id = r.u32();
        e.owner = r.enumeration<Owner>();
        e.kind = r.enumeration<SpellKind>();
        e.position = r.point();
        e.remaining = r.number();
        e.radius = r.number();
        e.target_id = r.u32();
        s.effects.push_back(e);
    }
    count = r.count(MaximumEntities);
    for (std::size_t i = 0; i < count && r.ok; ++i) {
        Projectile p;
        p.id = r.u32();
        p.owner = r.enumeration<Owner>();
        p.position = r.point();
        p.target_id = r.u32();
        p.damage = r.number();
        p.speed = r.number();
        auto siege = r.u8();
        if (siege > 1)
            r.ok = false;
        p.siege = siege != 0;
        s.projectiles.push_back(p);
    }
    count = r.count(80);
    for (std::size_t i = 0; i < count && r.ok; ++i) {
        Event e;
        e.tick = r.u64();
        e.owner = r.enumeration<Owner>();
        e.subject_id = r.u32();
        e.text = r.text(1024);
        s.events.push_back(std::move(e));
    }
    s.ai_timer = r.number();
    s.upkeep_timer = r.number();
    s.ai_phase = r.u32();
    if (legacy) {
        for (auto &faction : s.factions)
            faction.automatic_growth.enabled = false;
    } else {
        if (r.count(s.factions.size()) != s.factions.size())
            return fail("Invalid automatic growth faction count.");
        for (auto &faction : s.factions) {
            const auto enabled = r.u8();
            faction.automatic_growth.enabled = enabled != 0;
            faction.automatic_growth.protected_wood = r.number();
            faction.automatic_growth.protected_stone = r.number();
            faction.housing_demand_ticks = r.u32();
            faction.automatic_growth_cooldown_ticks = r.u32();
            if (enabled > 1 || !valid_growth_policy(faction.automatic_growth) ||
                faction.housing_demand_ticks > 100 || faction.automatic_growth_cooldown_ticks > 150 ||
                (!faction.automatic_growth.enabled && faction.housing_demand_ticks != 0))
                return fail("Invalid automatic growth policy or timer.");
        }
        const auto metadata_count = r.count(MaximumEntities);
        if (metadata_count != s.buildings.size())
            return fail("Incomplete automatic growth building metadata.");
        std::unordered_set<Id> metadata_ids;
        for (std::size_t i = 0; i < metadata_count && r.ok; ++i) {
            const Id id = r.u32();
            const double harvested_food = r.number();
            const auto last_damage_tick = r.u64();
            auto building = std::find_if(s.buildings.begin(), s.buildings.end(),
                                         [&](const auto &candidate) { return candidate.id == id; });
            if (building == s.buildings.end() || !metadata_ids.insert(id).second ||
                !std::isfinite(harvested_food) || harvested_food < 0 ||
                harvested_food > farm_milestone(*building) || last_damage_tick > s.tick)
                return fail("Invalid automatic growth building metadata.");
            building->harvested_food = harvested_food;
            building->last_damage_tick = last_damage_tick;
        }
    }
    if (!r.ok || r.index != raw.size())
        return fail("Truncated, oversized, or trailing save data.");
    auto bounded = [](double value, double low, double high) {
        return std::isfinite(value) && value >= low && value <= high;
    };
    auto position = [&](Vec2 p) {
        return finite(p) && p.x >= 0 && p.y >= 0 && p.x < s.width && p.y < s.height;
    };
    auto timer = [&](double t) { return bounded(t, 0, 10000); };
    if (s.tick > 1000000000000ULL || !s.rng || !s.next_id ||
        s.next_id >= std::numeric_limits<Id>::max() - MaximumEntities ||
        !bounded(s.accumulator, 0, FixedStep - 1e-12) ||
        static_cast<unsigned>(s.ai_personality) > 2 || static_cast<unsigned>(s.difficulty) > 2 ||
        static_cast<unsigned>(s.outcome) > 2 || !bounded(s.ai_timer, -FixedStep, 100) ||
        !bounded(s.upkeep_timer, 0, 5.1))
        return fail("Invalid simulation clock, random state, or settings.");
    if (s.terrain.size() != 64 * 48)
        return fail("Invalid terrain length.");
    for (auto t : s.terrain)
        if (static_cast<unsigned>(t) > 5)
            return fail("Unknown terrain type.");
    for (auto p : s.crossing_sites)
        if (!position(p))
            return fail("Invalid crossing site.");
    for (auto p : s.objective_sites)
        if (!position(p))
            return fail("Invalid objective site.");
    for (int f = 0; f < 2; ++f)
        for (const auto *mask : {&s.explored[f], &s.visible[f]}) {
            if (mask->size() != 64 * 48)
                return fail("Invalid fog mask size.");
            for (auto v : *mask)
                if (v > 1)
                    return fail("Invalid fog mask value.");
        }
    for (const auto &f : s.factions) {
        if (!valid_resources(f.resources) || !valid_resources(f.capacity) ||
            !bounded(f.morale, 20, 100) || !timer(f.objective_hold) || !timer(f.research_timer) ||
            static_cast<unsigned>(f.research) > 5)
            return fail("Invalid faction state.");
        for (auto t : f.technologies)
            if (t > 2)
                return fail("Invalid technology level.");
        if (f.research != Technology::Count &&
            f.technologies[static_cast<std::size_t>(f.research)] >= 2)
            return fail("Cannot research a technology already at its maximum level.");
    }
    std::unordered_set<Id> identifiers;
    auto id_valid = [&](Id id) {
        return id > 0 && id < s.next_id && identifiers.insert(id).second;
    };
    for (const auto &b : s.buildings) {
        if (!id_valid(b.id) || static_cast<unsigned>(b.owner) > 1 ||
            static_cast<unsigned>(b.kind) >= 19 || !position(b.position) || b.rotation > 3 ||
            b.level < 1 || b.level > 3 || !bounded(b.health, 0, max_health(b)) ||
            !bounded(b.progress, 0, 1) || !timer(b.production_timer) || !timer(b.train_timer) ||
            !timer(b.attack_timer) || !finite(b.rally) || std::abs(b.rally.x) > 10000 ||
            std::abs(b.rally.y) > 10000)
            return fail("Invalid building data or duplicate identifier.");
        Rect rect = footprint(b.kind, b.position, b.rotation);
        if (rect.x < 1 || rect.y < 1 || rect.x + rect.w >= s.width || rect.y + rect.h >= s.height)
            return fail("Building footprint exceeds map bounds.");
        for (auto kind : b.train_queue)
            if (static_cast<unsigned>(kind) >= 12 || definition(kind).recruited_at != b.kind)
                return fail("Invalid recruitment queue.");
    }
    for (const auto &u : s.units) {
        if (!id_valid(u.id) || static_cast<unsigned>(u.owner) > 1 ||
            static_cast<unsigned>(u.kind) >= 12 || !position(u.position) ||
            !position(u.destination) || !position(u.patrol_origin) ||
            !position(u.patrol_destination) || std::abs(u.destination.x) > 10000 ||
            std::abs(u.destination.y) > 10000 || !bounded(u.health, 0, max_health(u)) ||
            !timer(u.attack_timer) || !timer(u.work_timer) || !timer(u.repath_timer) ||
            !bounded(u.illusion_seconds, 0, 20) || static_cast<unsigned>(u.order) > 7 ||
            static_cast<unsigned>(u.stance) > 2 || static_cast<unsigned>(u.cargo_kind) > 4 ||
            !bounded(u.cargo, 0, 12.00001) || u.path_index > u.path.size() ||
            u.target_id >= s.next_id || u.job_id >= s.next_id)
            return fail("Invalid unit data or duplicate identifier.");
        for (auto p : u.path)
            if (!position(p))
                return fail("Invalid navigation path.");
        for (auto cd : u.spell_cooldowns)
            if (!bounded(cd, 0, 100))
                return fail("Invalid spell cooldown.");
    }
    for (const auto &n : s.nodes)
        if (!id_valid(n.id) || static_cast<unsigned>(n.kind) > 4 || !position(n.position) ||
            !bounded(n.remaining, 0, 1000000))
            return fail("Invalid resource node.");
    for (const auto &e : s.effects)
        if (!id_valid(e.id) || static_cast<unsigned>(e.owner) > 1 ||
            static_cast<unsigned>(e.kind) >= 9 || !position(e.position) || !timer(e.remaining) ||
            !bounded(e.radius, 0.1, 20) || e.target_id >= s.next_id)
            return fail("Invalid spell effect.");
    for (const auto &p : s.projectiles)
        if (!id_valid(p.id) || static_cast<unsigned>(p.owner) > 1 || !position(p.position) ||
            p.target_id >= s.next_id || !bounded(p.damage, 0, 1000) || !bounded(p.speed, 0.1, 100))
            return fail("Invalid projectile.");
    for (const auto &e : s.events)
        if (e.tick > s.tick || static_cast<unsigned>(e.owner) > 2 || e.subject_id >= s.next_id)
            return fail("Invalid event log.");
    // Commit only after the complete candidate validates. Failed loads preserve
    // the exact previous match, including RNG, fog, orders, and queued recruits.
    state_ = std::move(s);
    return {true, ""};
}
} // namespace wildlight
