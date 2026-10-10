#pragma once

// Engine-independent authoritative simulation. All distances are grid cells;
// coordinates point to cell centres. Unreal presentation uses 200 cm per cell.
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wildlight {
using Id = std::uint32_t;
constexpr double FixedStep = 0.1;
constexpr std::uint32_t SaveVersion = 2;

struct Vec2 {
    double x = 0;
    double y = 0;
};
enum class Owner : std::uint8_t { Player, Enemy, Neutral };
enum class ResourceKind : std::uint8_t { Wood, Stone, Food, Gold, Mana };
enum class Terrain : std::uint8_t { Grass, Forest, Water, Rock, Ruin, Bridge };
enum class BuildingKind : std::uint8_t {
    Castle,
    House,
    Farm,
    Mill,
    Depot,
    LumberCamp,
    Quarry,
    GoldMine,
    Barracks,
    ArcherRange,
    Forge,
    Market,
    Wall,
    Gate,
    Watchtower,
    Beacon,
    Sanctuary,
    MageTower,
    SiegeWorkshop,
    Count
};
enum class UnitKind : std::uint8_t {
    Worker,
    Militia,
    Spearman,
    Swordsman,
    Archer,
    Cavalry,
    Engineer,
    ArcaneGuard,
    ForestAlly,
    Ram,
    Catapult,
    Wizard,
    Count
};
enum class MagicSchool : std::uint8_t { Roots, Light, Shadows };
enum class SpellKind : std::uint8_t {
    RootBarrier,
    Heal,
    HarvestBloom,
    LightBridge,
    Amplify,
    Shield,
    Veil,
    Disrupt,
    Illusion,
    Count
};
enum class AIPersonality : std::uint8_t { Builder, Conqueror, Arcanist };
enum class Difficulty : std::uint8_t { Easy, Normal, Hard };
enum class CommandType : std::uint8_t {
    Build,
    Move,
    Attack,
    Gather,
    CastSpell,
    Repair,
    Upgrade,
    Demolish,
    Train,
    Stop,
    Patrol,
    SetRally,
    Research,
    Trade,
    SetStance,
    Surrender,
    SetAutomaticGrowth
};
enum class OrderType : std::uint8_t {
    Idle,
    Move,
    Attack,
    Gather,
    ReturnCargo,
    Build,
    Repair,
    Patrol
};
enum class Stance : std::uint8_t { Defensive, Aggressive, Hold };
enum class Technology : std::uint8_t { Forestry, Agriculture, Masonry, Steel, Arcana, Count };
enum class MatchOutcome : std::uint8_t { Playing, PlayerVictory, EnemyVictory };

struct Resources {
    double wood = 0, stone = 0, food = 0, gold = 0, mana = 0;
    double get(ResourceKind kind) const;
    void add(ResourceKind kind, double amount);
    bool can_afford(const Resources &cost) const;
};
struct AutomaticGrowthPolicy {
    bool enabled = true;
    double protected_wood = 100;
    double protected_stone = 50;
};
struct BuildingDefinition {
    BuildingKind kind;
    std::string_view name;
    Resources cost;
    int width, height;
    double health, build_seconds;
    int population, storage, worker_slots;
    double light_radius;
    bool requires_light;
};
struct UnitDefinition {
    UnitKind kind;
    std::string_view name;
    Resources cost;
    BuildingKind recruited_at;
    double health, damage, armor, range, speed, cooldown, train_seconds;
    int population;
    bool siege;
};
struct SpellDefinition {
    SpellKind kind;
    MagicSchool school;
    std::string_view name;
    double mana, duration, cooldown, radius, cast_range;
};
struct MapDefinition {
    std::string_view name, description;
    int width, height;
    Vec2 player_start, enemy_start;
    int objective_beacons;
    double objective_seconds;
};
const std::array<BuildingDefinition, 19> &building_catalog();
const std::array<UnitDefinition, 12> &unit_catalog();
const std::array<SpellDefinition, 9> &spell_catalog();
const std::array<MapDefinition, 3> &map_catalog();
const BuildingDefinition &definition(BuildingKind kind);
const UnitDefinition &definition(UnitKind kind);
const SpellDefinition &definition(SpellKind kind);

struct Building {
    Id id = 0;
    Owner owner = Owner::Neutral;
    BuildingKind kind = BuildingKind::House;
    Vec2 position;
    std::uint8_t rotation = 0;
    double health = 0, progress = 0;
    std::uint8_t level = 1;
    bool connected = false;
    double production_timer = 0, train_timer = 0, attack_timer = 0;
    Vec2 rally;
    std::vector<UnitKind> train_queue;
    double harvested_food = 0;
    std::uint64_t last_damage_tick = 0;
};
Resources building_upgrade_cost(const Building &building);
struct Unit {
    Id id = 0;
    Owner owner = Owner::Neutral;
    UnitKind kind = UnitKind::Worker;
    Vec2 position, destination, patrol_origin;
    double health = 0, attack_timer = 0, work_timer = 0;
    OrderType order = OrderType::Idle;
    Stance stance = Stance::Defensive;
    Id target_id = 0, job_id = 0;
    ResourceKind cargo_kind = ResourceKind::Wood;
    double cargo = 0;
    std::vector<Vec2> path;
    std::uint32_t path_index = 0;
    double repath_timer = 0;
    std::array<double, 9> spell_cooldowns{};
    double illusion_seconds = 0; // Temporary decoys never count toward population.
    bool automatic_work = true;  // Explicit Gather/Stop orders opt out of job assignment.
    bool patrolling = false;
    Vec2 patrol_destination;
};
struct ResourceNode {
    Id id = 0;
    ResourceKind kind = ResourceKind::Wood;
    Vec2 position;
    double remaining = 0;
};
struct SpellEffect {
    Id id = 0;
    Owner owner = Owner::Neutral;
    SpellKind kind = SpellKind::RootBarrier;
    Vec2 position;
    double remaining = 0, radius = 0;
    Id target_id = 0;
};
struct Projectile {
    Id id = 0;
    Owner owner = Owner::Neutral;
    Vec2 position;
    Id target_id = 0;
    double damage = 0, speed = 0;
    bool siege = false;
};
struct FactionState {
    Resources resources, capacity;
    int population = 0, population_limit = 0;
    double morale = 100, objective_hold = 0;
    std::array<std::uint8_t, 5> technologies{};
    Technology research = Technology::Count;
    double research_timer = 0;
    std::uint32_t units_lost = 0, units_recruited = 0, buildings_lost = 0;
    AutomaticGrowthPolicy automatic_growth;
    std::uint32_t housing_demand_ticks = 0;
    std::uint32_t automatic_growth_cooldown_ticks = 0;
};
struct Event {
    std::uint64_t tick = 0;
    Owner owner = Owner::Neutral;
    Id subject_id = 0;
    std::string text;
};
struct State {
    std::uint64_t tick = 0;
    double accumulator = 0;
    std::uint32_t rng = 1, next_id = 1, map_index = 0;
    int width = 0, height = 0;
    AIPersonality ai_personality = AIPersonality::Builder;
    Difficulty difficulty = Difficulty::Normal;
    MatchOutcome outcome = MatchOutcome::Playing;
    std::vector<Terrain> terrain;
    std::vector<Vec2> crossing_sites, objective_sites;
    std::array<std::vector<std::uint8_t>, 2> explored, visible;
    std::array<FactionState, 2> factions;
    std::vector<Building> buildings;
    std::vector<Unit> units;
    std::vector<ResourceNode> nodes;
    std::vector<SpellEffect> effects;
    std::vector<Projectile> projectiles;
    std::vector<Event> events;
    double ai_timer = 0, upkeep_timer = 0;
    std::uint32_t ai_phase = 0;
};
struct Command {
    CommandType type = CommandType::Move;
    Owner owner = Owner::Player;
    std::vector<Id> ids;
    Id target_id = 0;
    Vec2 position;
    BuildingKind building = BuildingKind::House;
    UnitKind unit = UnitKind::Worker;
    SpellKind spell = SpellKind::RootBarrier;
    Technology technology = Technology::Forestry;
    ResourceKind resource = ResourceKind::Wood;
    double amount = 50;
    std::uint8_t rotation = 0;
    Stance stance = Stance::Defensive;
    AutomaticGrowthPolicy automatic_growth;
};
struct CommandResult {
    bool accepted = false;
    std::string reason;
    Id created_id = 0;
    explicit operator bool() const { return accepted; }
};
struct LoadResult {
    bool accepted = false;
    std::string reason;
    explicit operator bool() const { return accepted; }
};

class Simulation {
  public:
    Simulation();
    void new_match(std::uint32_t map_index = 0, std::uint32_t seed = 1,
                   AIPersonality personality = AIPersonality::Builder,
                   Difficulty difficulty = Difficulty::Normal);
    const State &state() const { return state_; }
    const Building *building(Id id) const;
    const Unit *unit(Id id) const;
    const ResourceNode *node(Id id) const;
    CommandResult issue(const Command &command);
    CommandResult can_build(Owner owner, BuildingKind kind, Vec2 position,
                            std::uint8_t rotation = 0) const;
    bool walkable(Vec2 position, Owner owner = Owner::Player) const;
    std::vector<Vec2> find_path(Vec2 from, Vec2 to, Owner owner) const;
    bool has_light(Owner owner, Vec2 position) const;
    bool is_visible(Owner owner, Vec2 position) const;
    bool is_unit_visible(Owner viewer, Id unit_id) const;
    double day_fraction() const;
    std::string automatic_growth_status(Id building_id) const;
    void step(double seconds);
    std::string serialize() const;
    LoadResult deserialize(const std::string &bytes);

  private:
    State state_;
    void fixed_step();
    void update_network();
    void update_economy();
    void update_units();
    void update_projectiles();
    void update_effects();
    void update_ai();
    void update_visibility();
    void update_outcome();
    void assign_jobs();
    void recalculate_factions();
    void update_automatic_growth();
    std::string automatic_growth_reason(const Building &building) const;
    bool upgrade_building(Building &building);
    void event(Owner owner, Id subject, const std::string &text);
    void damage(Id target, double amount, bool siege, Owner source);
    void order_path(Unit &unit, Vec2 destination);
    Building *mutable_building(Id id);
    Unit *mutable_unit(Id id);
    ResourceNode *mutable_node(Id id);
    Id add_building(Owner owner, BuildingKind kind, Vec2 position, bool completed);
    Id add_unit(Owner owner, UnitKind kind, Vec2 position);
    std::uint32_t random();
};
} // namespace wildlight
