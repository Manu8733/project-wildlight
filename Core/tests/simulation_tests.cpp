#include "wildlight/Simulation.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace {
using namespace wildlight;
int assertions = 0;

#define CHECK(condition) do { ++assertions; if (!(condition)) { \
    std::cerr << __func__ << ':' << __LINE__ << ": " << #condition << '\n'; \
    return false; } } while (false)

bool near(double a, double b, double tolerance = 1e-6) {
    return std::abs(a - b) <= tolerance;
}
double distance(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }
void advance(Simulation& sim, int ticks) {
    for (int i = 0; i < ticks; ++i) sim.step(FixedStep);
}
Id first_unit(const Simulation& sim, Owner owner, UnitKind kind) {
    for (const auto& unit : sim.state().units)
        if (unit.owner == owner && unit.kind == kind && unit.health > 0) return unit.id;
    return 0;
}
Id first_building(const Simulation& sim, Owner owner, BuildingKind kind) {
    for (const auto& building : sim.state().buildings)
        if (building.owner == owner && building.kind == kind && building.health > 0)
            return building.id;
    return 0;
}
Id first_node(const Simulation& sim, ResourceKind kind, Vec2 closest) {
    Id best_id = 0;
    double best = std::numeric_limits<double>::max();
    for (const auto& node : sim.state().nodes)
        if (node.kind == kind && node.remaining > 0 && distance(node.position, closest) < best) {
            best_id = node.id;
            best = distance(node.position, closest);
        }
    return best_id;
}
bool wait_complete(Simulation& sim, Id id, int ticks = 450) {
    for (int i = 0; i < ticks; ++i) {
        const auto* building = sim.building(id);
        if (!building) return false;
        if (building->progress >= 1) return true;
        sim.step(FixedStep);
    }
    return sim.building(id) && sim.building(id)->progress >= 1;
}
bool wait_position(Simulation& sim, Id id, Vec2 destination, int ticks = 600) {
    for (int i = 0; i < ticks; ++i) {
        const auto* unit = sim.unit(id);
        if (!unit) return false;
        if (distance(unit->position, destination) < 0.6) return true;
        sim.step(FixedStep);
    }
    return sim.unit(id) && distance(sim.unit(id)->position, destination) < 0.6;
}
Command unit_command(CommandType type, Id id, Vec2 position = {}, Owner owner = Owner::Player) {
    Command command;
    command.type = type; command.ids = {id}; command.position = position; command.owner = owner;
    return command;
}
Id place_near_castle(Simulation& sim, BuildingKind kind, Owner owner = Owner::Player) {
    const auto* castle = sim.building(first_building(sim, owner, BuildingKind::Castle));
    if (!castle) return 0;
    const Vec2 origin = castle->position;
    // Try compact settlements first, without depending on hard-coded map decoration.
    for (int radius = 3; radius < 17; ++radius)
        for (int dy = -radius; dy <= radius; ++dy)
            for (int dx = -radius; dx <= radius; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != radius) continue;
                const Vec2 position{origin.x + dx, origin.y + dy};
                if (!sim.can_build(owner, kind, position)) continue;
                Command command;
                command.type = CommandType::Build; command.owner = owner;
                command.building = kind; command.position = position;
                const auto result = sim.issue(command);
                if (result) return result.created_id;
            }
    return 0;
}
bool reject_without_mutation(Simulation& sim, const Command& command) {
    const std::string before = sim.serialize();
    const auto result = sim.issue(command);
    return !result && !result.reason.empty() && before == sim.serialize();
}
bool valid_resources(const Resources& resources) {
    for (const auto kind : {ResourceKind::Wood, ResourceKind::Stone, ResourceKind::Food,
                            ResourceKind::Gold, ResourceKind::Mana})
        if (!std::isfinite(resources.get(kind)) || resources.get(kind) < -1e-8) return false;
    return true;
}

// Version-1 persistence regression helpers intentionally exercise the file
// boundary with correctly checksummed invalid payloads. Otherwise corruption
// tests would only prove that the checksum parser runs, not that data is valid.
std::string raw_save(const std::string& saved) {
    const auto newline = saved.find('\n');
    if (newline == std::string::npos) return {};
    const auto nibble = [](char value) {
        return value >= '0' && value <= '9' ? value - '0' : value - 'a' + 10;
    };
    std::string raw;
    for (std::size_t i = newline + 1; i + 1 < saved.size(); i += 2)
        raw.push_back(static_cast<char>(nibble(saved[i]) * 16 + nibble(saved[i + 1])));
    return raw;
}
std::string wrap_save(const std::string& raw) {
    std::uint32_t checksum = 2166136261u;
    for (const unsigned char byte : raw) { checksum ^= byte; checksum *= 16777619u; }
    std::string saved = "WILDLIGHT " + std::to_string(SaveVersion) + " " + std::to_string(checksum) + "\n";
    constexpr char hex[] = "0123456789abcdef";
    for (const unsigned char byte : raw) { saved += hex[byte >> 4]; saved += hex[byte & 15]; }
    return saved;
}
void write_u32(std::string& raw, std::size_t offset, std::uint32_t value) {
    for (int byte = 0; byte < 4; ++byte) raw[offset + byte] = static_cast<char>(value >> (8 * byte));
}
void write_double(std::string& raw, std::size_t offset, double value) {
    const auto bits = std::bit_cast<std::uint64_t>(value);
    for (int byte = 0; byte < 8; ++byte) raw[offset + byte] = static_cast<char>(bits >> (8 * byte));
}
std::uint32_t read_u32(const std::string& raw, std::size_t offset) {
    std::uint32_t result = 0;
    for (int byte = 0; byte < 4; ++byte)
        result |= static_cast<std::uint32_t>(static_cast<unsigned char>(raw[offset + byte])) << (8 * byte);
    return result;
}

bool initial_state_and_ids() {
    Simulation sim;
    CHECK(sim.state().width == 64 && sim.state().height == 48);
    CHECK(sim.state().terrain.size() == 64 * 48);
    CHECK(sim.state().outcome == MatchOutcome::Playing);
    CHECK(sim.state().factions[0].population == 13);
    CHECK(sim.state().factions[0].population_limit == 18);
    std::set<Id> ids;
    for (const auto& building : sim.state().buildings) CHECK(building.id && ids.insert(building.id).second);
    for (const auto& unit : sim.state().units) CHECK(unit.id && ids.insert(unit.id).second);
    for (const auto& node : sim.state().nodes) CHECK(node.id && ids.insert(node.id).second);
    CHECK(sim.state().next_id > *ids.rbegin());
    CHECK(sim.building(0) == nullptr && sim.unit(0) == nullptr && sim.node(0) == nullptr);
    CHECK(sim.is_visible(Owner::Player, map_catalog()[0].player_start));
    CHECK(!sim.is_visible(Owner::Player, map_catalog()[0].enemy_start));
    CHECK(!sim.is_visible(Owner::Neutral, map_catalog()[0].player_start));
    return true;
}

bool deterministic_seed_and_commands() {
    Simulation first, second, other_seed;
    first.new_match(1, 12345, AIPersonality::Arcanist, Difficulty::Hard);
    second.new_match(1, 12345, AIPersonality::Arcanist, Difficulty::Hard);
    other_seed.new_match(1, 67890, AIPersonality::Arcanist, Difficulty::Hard);
    CHECK(first.serialize() == second.serialize());
    CHECK(first.serialize() != other_seed.serialize());
    const Id first_id = place_near_castle(first, BuildingKind::Farm);
    const Id second_id = place_near_castle(second, BuildingKind::Farm);
    CHECK(first_id && first_id == second_id);
    advance(first, 400);
    advance(second, 400);
    CHECK(first.serialize() == second.serialize());
    return true;
}

bool fixed_step_partition() {
    Simulation full, halves;
    for (int i = 0; i < 100; ++i) {
        full.step(FixedStep);
        halves.step(FixedStep / 2);
        halves.step(FixedStep / 2);
    }
    CHECK(full.state().tick == 100);
    CHECK(full.serialize() == halves.serialize());
    const auto before = full.serialize();
    full.step(-1); full.step(0);
    full.step(std::numeric_limits<double>::quiet_NaN());
    full.step(std::numeric_limits<double>::infinity());
    CHECK(full.serialize() == before);
    return true;
}

bool command_validation_is_atomic() {
    Simulation sim;
    const Id worker = first_unit(sim, Owner::Player, UnitKind::Worker);
    const Id enemy = first_unit(sim, Owner::Enemy, UnitKind::Worker);
    auto command = unit_command(CommandType::Move, worker, {15.5, 25.5});
    command.ids.push_back(enemy);
    CHECK(reject_without_mutation(sim, command));
    command.ids = {worker, 999999};
    CHECK(reject_without_mutation(sim, command));
    command.ids = {worker}; command.owner = Owner::Neutral;
    CHECK(reject_without_mutation(sim, command));
    command.owner = static_cast<Owner>(255);
    CHECK(reject_without_mutation(sim, command));
    command.owner = Owner::Player; command.type = static_cast<CommandType>(255);
    CHECK(reject_without_mutation(sim, command));
    command.type = CommandType::Move; command.position.x = std::numeric_limits<double>::quiet_NaN();
    CHECK(reject_without_mutation(sim, command));
    command.position = {std::numeric_limits<double>::infinity(), 10};
    CHECK(reject_without_mutation(sim, command));
    command.position = {-1, 10};
    CHECK(reject_without_mutation(sim, command));
    command.position = {1e100, 10};
    CHECK(reject_without_mutation(sim, command));
    command.type = CommandType::Build; command.building = BuildingKind::Count;
    command.position = {15.5, 24.5};
    CHECK(reject_without_mutation(sim, command));
    command.building = BuildingKind::House; command.rotation = 4;
    CHECK(reject_without_mutation(sim, command));
    command.rotation = 0; command.position = {1e100, 10};
    CHECK(reject_without_mutation(sim, command));
    command.type = CommandType::Train; command.unit = UnitKind::Count;
    command.position = {}; command.target_id = first_building(sim, Owner::Player, BuildingKind::Castle);
    CHECK(reject_without_mutation(sim, command));
    command.type = CommandType::CastSpell; command.spell = SpellKind::Count;
    CHECK(reject_without_mutation(sim, command));
    command.type = CommandType::Research; command.technology = Technology::Count;
    CHECK(reject_without_mutation(sim, command));
    command.type = CommandType::Trade; command.resource = static_cast<ResourceKind>(255);
    CHECK(reject_without_mutation(sim, command));
    return true;
}

bool hostile_targets_need_sight_and_ownership() {
    Simulation sim;
    const Id militia = first_unit(sim, Owner::Player, UnitKind::Militia);
    Command attack = unit_command(CommandType::Attack, militia);
    attack.target_id = first_unit(sim, Owner::Enemy, UnitKind::Militia);
    CHECK(reject_without_mutation(sim, attack));
    attack.target_id = first_unit(sim, Owner::Player, UnitKind::Worker);
    CHECK(reject_without_mutation(sim, attack));
    attack.target_id = 999999;
    CHECK(reject_without_mutation(sim, attack));
    Command demolish; demolish.type = CommandType::Demolish;
    demolish.target_id = first_building(sim, Owner::Enemy, BuildingKind::Castle);
    CHECK(reject_without_mutation(sim, demolish));
    demolish.target_id = first_building(sim, Owner::Player, BuildingKind::Castle);
    CHECK(reject_without_mutation(sim, demolish));
    return true;
}

bool building_cost_construction_and_cancellation() {
    Simulation sim;
    const auto before = sim.state().factions[0].resources;
    const Id house = place_near_castle(sim, BuildingKind::House);
    CHECK(house);
    CHECK(sim.building(house)->progress == 0);
    const auto after = sim.state().factions[0].resources;
    CHECK(near(before.wood - after.wood, definition(BuildingKind::House).cost.wood));
    CHECK(near(before.stone - after.stone, definition(BuildingKind::House).cost.stone));
    CHECK(sim.state().factions[0].population_limit == 18);
    Command overlap; overlap.type = CommandType::Build; overlap.building = BuildingKind::House;
    overlap.position = sim.building(house)->position;
    CHECK(reject_without_mutation(sim, overlap));
    CHECK(wait_complete(sim, house));
    CHECK(sim.state().factions[0].population_limit == 26);
    const Id second_house = place_near_castle(sim, BuildingKind::House);
    CHECK(second_house);
    const double wood = sim.state().factions[0].resources.wood;
    Command cancel; cancel.type = CommandType::Demolish; cancel.target_id = second_house;
    CHECK(sim.issue(cancel));
    CHECK(!sim.building(second_house));
    CHECK(near(sim.state().factions[0].resources.wood - wood,
               definition(BuildingKind::House).cost.wood * 0.75));
    return true;
}

bool recruitment_reserves_population_and_cost() {
    Simulation sim;
    const Id castle = first_building(sim, Owner::Player, BuildingKind::Castle);
    Command train; train.type = CommandType::Train; train.target_id = castle; train.unit = UnitKind::Worker;
    const double food = sim.state().factions[0].resources.food;
    CHECK(sim.issue(train));
    CHECK(near(food - sim.state().factions[0].resources.food, definition(UnitKind::Worker).cost.food));
    CHECK(sim.state().factions[0].population == 14);
    CHECK(sim.building(castle)->train_queue.size() == 1);
    for (int i = 0; i < 4; ++i) CHECK(sim.issue(train));
    CHECK(sim.state().factions[0].population == sim.state().factions[0].population_limit);
    CHECK(reject_without_mutation(sim, train));
    const auto before = sim.state().units.size();
    advance(sim, 65);
    CHECK(sim.state().units.size() > before);
    CHECK(sim.state().factions[0].units_recruited == 1);
    CHECK(sim.state().factions[0].population <= sim.state().factions[0].population_limit);
    return true;
}

bool path_uses_ford_and_avoids_obstacles() {
    Simulation sim;
    const auto path = sim.find_path({18.5, 11.5}, {44.5, 11.5}, Owner::Player);
    CHECK(!path.empty());
    bool used_ford = false;
    Vec2 previous{18.5, 11.5};
    for (const auto point : path) {
        CHECK(sim.walkable(point));
        CHECK(distance(previous, point) <= 1.415);
        if (point.x >= 30 && point.x < 34) {
            CHECK(point.y >= 22 && point.y < 26);
            used_ford = true;
        }
        previous = point;
    }
    CHECK(used_ford);
    CHECK(distance(path.back(), {44.5, 11.5}) < 0.01);
    CHECK(sim.find_path({-1, 0}, {10, 10}, Owner::Player).empty());
    CHECK(!sim.walkable({32.5, 11.5}));
    CHECK(!sim.walkable({std::numeric_limits<double>::quiet_NaN(), 10}));
    return true;
}

bool movement_and_duplicate_selection() {
    Simulation single, duplicate;
    const Id id = first_unit(single, Owner::Player, UnitKind::Militia);
    auto stance = unit_command(CommandType::SetStance, id); stance.stance = Stance::Hold;
    CHECK(single.issue(stance) && duplicate.issue(stance));
    auto move = unit_command(CommandType::Move, id, {18.5, 20.5});
    CHECK(single.issue(move));
    move.ids.push_back(id);
    CHECK(duplicate.issue(move));
    advance(single, 60); advance(duplicate, 60);
    CHECK(single.serialize() == duplicate.serialize());
    CHECK(single.unit(id));
    CHECK(distance(single.unit(id)->position, {18.5, 20.5}) < 0.6);
    CHECK(single.walkable(single.unit(id)->position));
    CHECK(single.unit(id)->order == OrderType::Idle);
    return true;
}

bool worker_logistics_deplete_and_deliver() {
    Simulation sim;
    const Id worker = first_unit(sim, Owner::Player, UnitKind::Worker);
    const Id node_id = first_node(sim, ResourceKind::Wood, sim.unit(worker)->position);
    CHECK(node_id);
    const double remaining = sim.node(node_id)->remaining;
    const double wood = sim.state().factions[0].resources.wood;
    auto gather = unit_command(CommandType::Gather, worker); gather.target_id = node_id;
    CHECK(sim.issue(gather));
    advance(sim, 300);
    CHECK(sim.node(node_id)->remaining < remaining);
    CHECK(sim.state().factions[0].resources.wood > wood);
    CHECK(sim.unit(worker) && sim.unit(worker)->cargo >= 0 && sim.unit(worker)->cargo <= 12.000001);
    for (const auto& faction : sim.state().factions) {
        CHECK(valid_resources(faction.resources));
        CHECK(faction.resources.wood <= faction.capacity.wood + 1e-8);
    }
    auto illegal = unit_command(CommandType::Gather, first_unit(sim, Owner::Player, UnitKind::Militia));
    illegal.target_id = node_id;
    CHECK(reject_without_mutation(sim, illegal));
    return true;
}

bool light_connection_and_demolition() {
    Simulation sim;
    const Id castle = first_building(sim, Owner::Player, BuildingKind::Castle);
    CHECK(sim.building(castle)->connected);
    CHECK(sim.has_light(Owner::Player, {20.5, 24.5}));
    CHECK(!sim.has_light(Owner::Player, {28.5, 24.5}));
    Command build; build.type = CommandType::Build; build.building = BuildingKind::Beacon;
    build.position = {21.5, 24.5};
    CHECK(sim.issue(build));
    const Id beacon = first_building(sim, Owner::Player, BuildingKind::Beacon);
    CHECK(beacon && !sim.building(beacon)->connected);
    CHECK(wait_complete(sim, beacon));
    CHECK(sim.building(beacon)->connected);
    CHECK(sim.has_light(Owner::Player, {28.5, 24.5}));
    Command demolish; demolish.type = CommandType::Demolish; demolish.target_id = beacon;
    CHECK(sim.issue(demolish));
    CHECK(!sim.has_light(Owner::Player, {28.5, 24.5}));
    CHECK(sim.building(castle)->connected);
    return true;
}

bool roots_cost_cooldown_and_expiration() {
    Simulation sim;
    const Id wizard = first_unit(sim, Owner::Player, UnitKind::Wizard);
    auto cast = unit_command(CommandType::CastSpell, wizard, {20.5, 24.5});
    cast.spell = SpellKind::RootBarrier;
    CHECK(sim.walkable(cast.position));
    const auto original_path = sim.find_path({17.5, 24.5}, {24.5, 24.5}, Owner::Player);
    const double mana = sim.state().factions[0].resources.mana;
    const auto result = sim.issue(cast);
    CHECK(result && result.created_id);
    CHECK(near(mana - sim.state().factions[0].resources.mana, definition(cast.spell).mana));
    CHECK(!sim.walkable(cast.position));
    const auto blocked_path = sim.find_path({17.5, 24.5}, {24.5, 24.5}, Owner::Player);
    CHECK(!blocked_path.empty() && blocked_path.size() > original_path.size());
    CHECK(reject_without_mutation(sim, cast));
    advance(sim, 181);
    CHECK(sim.walkable(cast.position));
    CHECK(reject_without_mutation(sim, cast));
    return true;
}

bool bridge_changes_route_and_expires_safely() {
    Simulation sim;
    const Id wizard = first_unit(sim, Owner::Player, UnitKind::Wizard);
    auto stance = unit_command(CommandType::SetStance, wizard); stance.stance = Stance::Hold;
    CHECK(sim.issue(stance));
    CHECK(sim.issue(unit_command(CommandType::Move, wizard, {27.5, 11.5})));
    CHECK(wait_position(sim, wizard, {27.5, 11.5}));
    const auto original_path = sim.find_path({27.5, 11.5}, {36.5, 11.5}, Owner::Player);
    CHECK(original_path.size() > 20);
    auto cast = unit_command(CommandType::CastSpell, wizard, {32, 11.5});
    cast.spell = SpellKind::LightBridge;
    const double mana = sim.state().factions[0].resources.mana;
    CHECK(sim.issue(cast));
    CHECK(near(mana - sim.state().factions[0].resources.mana, definition(cast.spell).mana));
    CHECK(sim.walkable({32.5, 11.5}));
    const auto bridge_path = sim.find_path({27.5, 11.5}, {36.5, 11.5}, Owner::Player);
    CHECK(!bridge_path.empty() && bridge_path.size() < original_path.size());
    CHECK(sim.issue(unit_command(CommandType::Move, wizard, {32.5, 11.5})));
    CHECK(wait_position(sim, wizard, {32.5, 11.5}, 60));
    advance(sim, 301);
    CHECK(!sim.walkable({32.5, 11.5}));
    CHECK(sim.unit(wizard) && sim.walkable(sim.unit(wizard)->position));
    CHECK(sim.find_path({27.5, 11.5}, {36.5, 11.5}, Owner::Player).size() > bridge_path.size());
    return true;
}

bool overlapping_roots_and_bridges_preserve_walkable_routes() {
    for (bool roots_first : {false, true}) {
        Simulation sim;
        const Id wizard = first_unit(sim, Owner::Player, UnitKind::Wizard);
        auto hold = unit_command(CommandType::SetStance, wizard); hold.stance = Stance::Hold;
        CHECK(sim.issue(hold));
        CHECK(sim.issue(unit_command(CommandType::Move, wizard, {27.5, 11.5})));
        CHECK(wait_position(sim, wizard, {27.5, 11.5}));
        auto roots = unit_command(CommandType::CastSpell, wizard, {29.5, 10.5});
        roots.spell = SpellKind::RootBarrier;
        auto bridge = unit_command(CommandType::CastSpell, wizard, {32, 11.5});
        bridge.spell = SpellKind::LightBridge;
        CHECK(sim.issue(roots_first ? roots : bridge));
        CHECK(sim.issue(roots_first ? bridge : roots));
        CHECK(!sim.walkable({30.5, 10.5}, Owner::Player));
        const auto path = sim.find_path({36.5, 10.5}, {30.5, 10.5}, Owner::Player);
        CHECK(!path.empty());
        for (const auto& point : path) CHECK(sim.walkable(point, Owner::Player));
    }
    return true;
}

bool invalid_spells_do_not_consume_mana() {
    Simulation sim;
    const Id wizard = first_unit(sim, Owner::Player, UnitKind::Wizard);
    auto cast = unit_command(CommandType::CastSpell, wizard, {15.5, 24.5});
    cast.spell = SpellKind::LightBridge;
    CHECK(reject_without_mutation(sim, cast));
    cast.spell = SpellKind::RootBarrier; cast.position = map_catalog()[0].player_start;
    CHECK(reject_without_mutation(sim, cast));
    cast.spell = SpellKind::Heal; cast.position = {55.5, 24.5};
    CHECK(reject_without_mutation(sim, cast));
    cast.position = {15.5, 24.5}; cast.ids = {first_unit(sim, Owner::Player, UnitKind::Worker)};
    CHECK(reject_without_mutation(sim, cast));
    return true;
}

bool enemy_disruption_breaks_and_restores_light() {
    Simulation sim;
    Command build; build.type = CommandType::Build; build.building = BuildingKind::Beacon;
    build.position = {21.5, 24.5};
    const auto placed = sim.issue(build);
    CHECK(placed && wait_complete(sim, placed.created_id));
    CHECK(sim.has_light(Owner::Player, {28.5, 24.5}));
    const Id wizard = first_unit(sim, Owner::Enemy, UnitKind::Wizard);
    auto hold = unit_command(CommandType::SetStance, wizard, {}, Owner::Enemy); hold.stance = Stance::Hold;
    CHECK(sim.issue(hold));
    CHECK(sim.issue(unit_command(CommandType::Move, wizard, {30.5, 24.5}, Owner::Enemy)));
    CHECK(wait_position(sim, wizard, {30.5, 24.5}));
    auto cast = unit_command(CommandType::CastSpell, wizard, build.position, Owner::Enemy);
    cast.spell = SpellKind::Disrupt; cast.target_id = placed.created_id;
    const double mana = sim.state().factions[1].resources.mana;
    CHECK(sim.issue(cast));
    CHECK(near(mana - sim.state().factions[1].resources.mana, definition(cast.spell).mana));
    CHECK(!sim.building(placed.created_id)->connected);
    CHECK(!sim.has_light(Owner::Player, {28.5, 24.5}));
    advance(sim, 241);
    CHECK(sim.building(placed.created_id) && sim.building(placed.created_id)->connected);
    CHECK(sim.has_light(Owner::Player, {28.5, 24.5}));
    return true;
}

bool illusions_expire_without_population_cost() {
    Simulation sim;
    const Id wizard = first_unit(sim, Owner::Player, UnitKind::Wizard);
    const auto population = sim.state().factions[0].population;
    auto cast = unit_command(CommandType::CastSpell, wizard, {20.5, 24.5});
    cast.spell = SpellKind::Illusion;
    CHECK(sim.issue(cast));
    int illusions = 0;
    for (const auto& unit : sim.state().units)
        if (unit.owner == Owner::Player && unit.illusion_seconds > 0) ++illusions;
    CHECK(illusions == 3);
    advance(sim, 1);
    CHECK(sim.state().factions[0].population == population);
    Simulation restored;
    CHECK(restored.deserialize(sim.serialize()));
    CHECK(restored.serialize() == sim.serialize());
    advance(sim, 201); advance(restored, 201);
    CHECK(sim.serialize() == restored.serialize());
    for (const auto& unit : sim.state().units) CHECK(unit.illusion_seconds == 0);
    CHECK(sim.state().factions[0].population == population);
    return true;
}

bool veil_hides_units_until_scouts_approach() {
    Simulation sim;
    const Id scout = first_unit(sim, Owner::Player, UnitKind::Wizard);
    const Id hidden = first_unit(sim, Owner::Enemy, UnitKind::Wizard);
    auto hold = unit_command(CommandType::SetStance, scout); hold.stance = Stance::Hold;
    CHECK(sim.issue(hold));
    hold.owner = Owner::Enemy; hold.ids = {hidden};
    CHECK(sim.issue(hold));
    CHECK(sim.issue(unit_command(CommandType::Move, scout, {24.5, 24.5})));
    CHECK(sim.issue(unit_command(CommandType::Move, hidden, {30.5, 24.5}, Owner::Enemy)));
    CHECK(wait_position(sim, scout, {24.5, 24.5}));
    CHECK(wait_position(sim, hidden, {30.5, 24.5}));
    CHECK(sim.is_unit_visible(Owner::Player, hidden));
    auto cast = unit_command(CommandType::CastSpell, hidden, {30.5, 24.5}, Owner::Enemy);
    cast.spell = SpellKind::Veil;
    CHECK(sim.issue(cast));
    CHECK(sim.is_visible(Owner::Player, sim.unit(hidden)->position));
    CHECK(!sim.is_unit_visible(Owner::Player, hidden));
    auto attack = unit_command(CommandType::Attack, scout); attack.target_id = hidden;
    CHECK(reject_without_mutation(sim, attack));
    CHECK(sim.issue(unit_command(CommandType::Move, scout, {28.5, 24.5})));
    CHECK(wait_position(sim, scout, {28.5, 24.5}, 40));
    CHECK(sim.is_unit_visible(Owner::Player, hidden));
    CHECK(sim.issue(attack));
    return true;
}

bool connected_objectives_award_player_victory() {
    Simulation sim;
    sim.new_match(2, 1, AIPersonality::Builder, Difficulty::Easy);
    const Vec2 positions[]{{19.5, 24.5}, {21.5, 13.5}, {31.5, 23.5}, {31.5, 6.5}};
    for (const auto position : positions) {
        Command build; build.type = CommandType::Build; build.building = BuildingKind::Beacon;
        build.position = position;
        const auto placed = sim.issue(build);
        CHECK(placed);
        CHECK(wait_complete(sim, placed.created_id, 700));
        CHECK(sim.building(placed.created_id)->connected);
    }
    for (int i = 0; i < 1000 && sim.state().outcome == MatchOutcome::Playing; ++i)
        sim.step(FixedStep);
    CHECK(sim.state().outcome == MatchOutcome::PlayerVictory);
    CHECK(sim.state().factions[0].objective_hold >= map_catalog()[2].objective_seconds - 1e-6);
    CHECK(first_building(sim, Owner::Enemy, BuildingKind::Castle));
    return true;
}

bool combat_damage_and_friendly_fire_rejection() {
    Simulation sim;
    const Id attacker = first_unit(sim, Owner::Player, UnitKind::Militia);
    const Id defender = first_unit(sim, Owner::Enemy, UnitKind::Militia);
    auto hold = unit_command(CommandType::SetStance, attacker); hold.stance = Stance::Hold;
    CHECK(sim.issue(hold));
    hold.owner = Owner::Enemy; hold.ids = {defender};
    CHECK(sim.issue(hold));
    CHECK(sim.issue(unit_command(CommandType::Move, attacker, {27.5, 23.5})));
    CHECK(sim.issue(unit_command(CommandType::Move, defender, {28.5, 23.5}, Owner::Enemy)));
    CHECK(wait_position(sim, attacker, {27.5, 23.5}));
    CHECK(wait_position(sim, defender, {28.5, 23.5}));
    CHECK(sim.is_visible(Owner::Player, sim.unit(defender)->position));
    const double health = sim.unit(defender)->health;
    auto attack = unit_command(CommandType::Attack, attacker); attack.target_id = defender;
    CHECK(sim.issue(attack));
    advance(sim, 80);
    CHECK(!sim.unit(defender) || sim.unit(defender)->health < health);
    auto friendly = unit_command(CommandType::Attack, attacker); friendly.target_id = first_unit(sim, Owner::Player, UnitKind::Worker);
    CHECK(reject_without_mutation(sim, friendly));
    return true;
}

bool save_roundtrip_and_continuation() {
    Simulation original, restored;
    original.new_match(2, 777, AIPersonality::Conqueror, Difficulty::Easy);
    CHECK(place_near_castle(original, BuildingKind::Farm));
    advance(original, 275);
    const auto saved = original.serialize();
    CHECK(!saved.empty());
    CHECK(restored.deserialize(saved));
    CHECK(restored.serialize() == saved);
    advance(original, 350); advance(restored, 350);
    CHECK(original.serialize() == restored.serialize());
    return true;
}

bool corrupted_saves_fail_atomically() {
    Simulation sim;
    CHECK(place_near_castle(sim, BuildingKind::House));
    advance(sim, 37);
    const auto before = sim.serialize();
    std::vector<std::string> invalid{
        {}, "not a Wildlight save", before.substr(0, before.size() / 2),
        before + "unexpected trailing data", std::string(8 * 1024 * 1024 + 1, 'x')
    };
    std::string corrupted = before;
    CHECK(corrupted.size() > 50);
    corrupted[corrupted.size() / 2] = corrupted[corrupted.size() / 2] == '0' ? '1' : '0';
    invalid.push_back(corrupted);
    for (const auto& bytes : invalid) {
        const auto result = sim.deserialize(bytes);
        CHECK(!result && !result.reason.empty());
        CHECK(sim.serialize() == before);
    }
    return true;
}

bool checksummed_invalid_save_data_is_rejected() {
    Simulation sim;
    const auto before = sim.serialize();
    const auto raw = raw_save(before);
    CHECK(raw.size() > 15000 && wrap_save(raw) == before);
    std::vector<std::string> invalid;
    const auto wrong_u32 = [&](std::size_t offset, std::uint32_t value) {
        auto changed = raw; write_u32(changed, offset, value); invalid.push_back(wrap_save(changed));
    };
    const auto wrong_double = [&](std::size_t offset, double value) {
        auto changed = raw; write_double(changed, offset, value); invalid.push_back(wrap_save(changed));
    };
    wrong_double(8, std::numeric_limits<double>::quiet_NaN()); // Accumulator
    wrong_double(8, -0.01);
    wrong_u32(16, 0); // RNG cannot be zero
    wrong_u32(20, 0); // Next stable identifier
    wrong_u32(24, 3); // Map outside catalog
    wrong_u32(28, 65); // Unsupported map dimensions
    wrong_u32(39, 0xffffffffu); // Oversized terrain vector
    auto changed = raw; changed[36] = static_cast<char>(255); invalid.push_back(wrap_save(changed));
    changed = raw; changed[43] = static_cast<char>(255); invalid.push_back(wrap_save(changed));

    // Advance over terrain, crossing/objective vectors, and the four fog masks.
    std::size_t cursor = 43 + read_u32(raw, 39);
    for (int i = 0; i < 2; ++i) cursor += 4 + read_u32(raw, cursor) * 16;
    for (int i = 0; i < 4; ++i) cursor += 4 + read_u32(raw, cursor);
    CHECK(cursor + 260 + 4 + 170 < raw.size());
    wrong_double(cursor, std::numeric_limits<double>::infinity()); // Player wood
    wrong_double(cursor, -1); // Resource underflow
    changed = raw; changed[cursor + 109] = static_cast<char>(255); invalid.push_back(wrap_save(changed));
    cursor += 260;
    CHECK(read_u32(raw, cursor) == 2);
    cursor += 4;
    wrong_double(cursor + 6, std::numeric_limits<double>::quiet_NaN()); // Building position
    wrong_double(cursor + 23, 1e100); // Building health beyond maximum
    wrong_double(cursor + 31, 2); // Construction progress beyond completion
    wrong_u32(cursor + 85, read_u32(raw, cursor)); // Second castle duplicates first ID
    changed = raw; changed[cursor + 5] = static_cast<char>(255); invalid.push_back(wrap_save(changed));
    invalid.push_back(wrap_save(raw + "trailing bytes"));
    for (const auto& bytes : invalid) {
        const auto result = sim.deserialize(bytes);
        CHECK(!result && !result.reason.empty());
        CHECK(sim.serialize() == before);
    }
    return true;
}

bool research_saves_reject_maxed_research_and_preserve_valid_states() {
    Simulation live;
    advance(live, 7);
    const auto before = live.serialize();
    const auto raw = raw_save(before);
    std::size_t cursor = 43 + read_u32(raw, 39);
    for (int i = 0; i < 2; ++i) cursor += 4 + read_u32(raw, cursor) * 16;
    for (int i = 0; i < 4; ++i) cursor += 4 + read_u32(raw, cursor);
    constexpr std::size_t faction_bytes = 130, technology_offset = 104;
    constexpr std::size_t research_offset = 109, research_timer_offset = 110;
    CHECK(cursor + 2 * faction_bytes <= raw.size());

    for (std::size_t faction = 0; faction < 2; ++faction) {
        const auto base = cursor + faction * faction_bytes;
        for (std::size_t technology = 0; technology < 5; ++technology) {
            for (double timer : {0.0, 35.0}) {
                auto invalid = raw;
                invalid[base + technology_offset + technology] = 2;
                invalid[base + research_offset] = static_cast<char>(technology);
                write_double(invalid, base + research_timer_offset, timer);
                const auto result = live.deserialize(wrap_save(invalid));
                CHECK(!result && !result.reason.empty());
                CHECK(live.serialize() == before);
            }
            for (std::uint8_t level : {std::uint8_t{0}, std::uint8_t{1}}) {
                for (double timer : {0.0, 35.0}) {
                    auto valid = raw;
                    valid[base + technology_offset + technology] = static_cast<char>(level);
                    valid[base + research_offset] = static_cast<char>(technology);
                    write_double(valid, base + research_timer_offset, timer);
                    const auto saved = wrap_save(valid);
                    Simulation restored;
                    CHECK(restored.deserialize(saved));
                    CHECK(restored.serialize() == saved);
                    restored.step(FixedStep);
                    const auto& state = restored.state().factions[faction];
                    CHECK(state.technologies[technology] == level + (timer == 0 ? 1 : 0));
                    CHECK(state.research == (timer == 0 ? Technology::Count : static_cast<Technology>(technology)));
                    Simulation continued;
                    CHECK(continued.deserialize(restored.serialize()));
                    CHECK(continued.serialize() == restored.serialize());
                }
            }
        }
    }

    auto completed = raw;
    for (std::size_t faction = 0; faction < 2; ++faction) {
        const auto base = cursor + faction * faction_bytes;
        for (std::size_t technology = 0; technology < 5; ++technology)
            completed[base + technology_offset + technology] = 2;
        completed[base + research_offset] = static_cast<char>(Technology::Count);
        write_double(completed, base + research_timer_offset, 0);
    }
    Simulation restored;
    const auto saved = wrap_save(completed);
    CHECK(restored.deserialize(saved));
    CHECK(restored.serialize() == saved);
    restored.step(FixedStep);
    for (const auto& faction : restored.state().factions)
        for (auto level : faction.technologies) CHECK(level == 2);
    return true;
}

bool research_upgrade_and_trade_progression() {
    Simulation sim;
    Command research; research.type = CommandType::Research; research.technology = Technology::Forestry;
    CHECK(reject_without_mutation(sim, research));
    const Id mill = place_near_castle(sim, BuildingKind::Mill);
    CHECK(mill && wait_complete(sim, mill));
    CHECK(sim.issue(research));
    CHECK(sim.state().factions[0].research == Technology::Forestry);
    CHECK(reject_without_mutation(sim, research));
    advance(sim, 351);
    CHECK(sim.state().factions[0].technologies[static_cast<std::size_t>(Technology::Forestry)] == 1);
    CHECK(sim.state().factions[0].research == Technology::Count);
    const Id house = place_near_castle(sim, BuildingKind::House);
    CHECK(house && wait_complete(sim, house));
    const int population_limit = sim.state().factions[0].population_limit;
    Command upgrade; upgrade.type = CommandType::Upgrade; upgrade.target_id = house;
    const double wood = sim.state().factions[0].resources.wood;
    CHECK(sim.issue(upgrade));
    CHECK(sim.building(house)->level == 2);
    CHECK(near(wood - sim.state().factions[0].resources.wood, definition(BuildingKind::House).cost.wood * 0.65));
    CHECK(sim.state().factions[0].population_limit == population_limit + definition(BuildingKind::House).population);
    const Id market = place_near_castle(sim, BuildingKind::Market);
    CHECK(market && wait_complete(sim, market));
    Command trade; trade.type = CommandType::Trade; trade.resource = ResourceKind::Wood; trade.amount = 50;
    const auto resources_before_trade = sim.state().factions[0].resources;
    CHECK(sim.issue(trade));
    CHECK(near(resources_before_trade.wood - sim.state().factions[0].resources.wood, 50));
    CHECK(near(sim.state().factions[0].resources.gold - resources_before_trade.gold, 22.5));
    trade.amount = std::numeric_limits<double>::quiet_NaN();
    CHECK(reject_without_mutation(sim, trade));
    trade.amount = -10;
    CHECK(reject_without_mutation(sim, trade));
    trade.amount = 501;
    CHECK(reject_without_mutation(sim, trade));
    return true;
}

bool normal_skirmish_reaches_castle_defeat() {
    Simulation sim;
    sim.new_match(0, 2718, AIPersonality::Conqueror, Difficulty::Normal);
    for (int i = 0; i < 6000 && sim.state().outcome == MatchOutcome::Playing; ++i)
        sim.step(FixedStep);
    CHECK(sim.state().outcome == MatchOutcome::EnemyVictory);
    CHECK(!first_building(sim, Owner::Player, BuildingKind::Castle));
    CHECK(sim.state().factions[0].buildings_lost >= 1);
    CHECK(sim.state().factions[0].units_lost > 0);
    CHECK(sim.state().factions[1].units_recruited > 0);
    for (const auto& faction : sim.state().factions) CHECK(valid_resources(faction.resources));
    const auto ended = sim.serialize();
    sim.step(1);
    CHECK(sim.serialize() == ended);
    Simulation restored;
    CHECK(restored.deserialize(ended));
    CHECK(restored.serialize() == ended);
    return true;
}

bool ai_personalities_obey_resource_rules() {
    for (const auto personality : {AIPersonality::Builder, AIPersonality::Conqueror, AIPersonality::Arcanist}) {
        Simulation sim;
        sim.new_match(0, 2718, personality, Difficulty::Normal);
        const auto initial_buildings = sim.state().buildings.size();
        advance(sim, 900);
        CHECK(sim.state().buildings.size() > initial_buildings);
        bool enemy_constructed = false;
        for (const auto& building : sim.state().buildings)
            if (building.owner == Owner::Enemy && building.kind != BuildingKind::Castle && building.progress >= 1)
                enemy_constructed = true;
        CHECK(enemy_constructed);
        for (const auto& faction : sim.state().factions) {
            CHECK(valid_resources(faction.resources));
            CHECK(faction.population >= 0 && faction.population_limit >= 0);
            CHECK(faction.morale >= 20 && faction.morale <= 100);
        }
    }
    return true;
}

bool surrender_closes_commands_and_saves_outcome() {
    for (const auto owner : {Owner::Player, Owner::Enemy}) {
        Simulation sim;
        Command surrender; surrender.type = CommandType::Surrender; surrender.owner = owner;
        CHECK(sim.issue(surrender));
        CHECK(sim.state().outcome == (owner == Owner::Player ? MatchOutcome::EnemyVictory : MatchOutcome::PlayerVictory));
        CHECK(reject_without_mutation(sim, surrender));
        Simulation restored;
        CHECK(restored.deserialize(sim.serialize()));
        CHECK(restored.state().outcome == sim.state().outcome);
        CHECK(restored.serialize() == sim.serialize());
    }
    return true;
}

struct Test { const char* name; bool (*run)(); };
} // namespace

int main() {
    const Test tests[]{
        {"initial state and stable IDs", initial_state_and_ids},
        {"seed and command determinism", deterministic_seed_and_commands},
        {"fixed-step partition and invalid time", fixed_step_partition},
        {"atomic command validation", command_validation_is_atomic},
        {"visibility and ownership", hostile_targets_need_sight_and_ownership},
        {"construction costs, completion and cancellation", building_cost_construction_and_cancellation},
        {"recruitment cost and population reservation", recruitment_reserves_population_and_cost},
        {"pathfinding over a permanent ford", path_uses_ford_and_avoids_obstacles},
        {"movement and duplicate selection", movement_and_duplicate_selection},
        {"worker gathering and physical cargo delivery", worker_logistics_deplete_and_deliver},
        {"light connection and disconnection", light_connection_and_demolition},
        {"root barrier mana, cooldown and expiry", roots_cost_cooldown_and_expiration},
        {"temporary bridge route and safe expiry", bridge_changes_route_and_expires_safely},
        {"overlapping roots and bridges keep routes walkable", overlapping_roots_and_bridges_preserve_walkable_routes},
        {"invalid spell atomicity", invalid_spells_do_not_consume_mana},
        {"enemy sabotage and light restoration", enemy_disruption_breaks_and_restores_light},
        {"illusions, population and save continuation", illusions_expire_without_population_cost},
        {"veil concealment and nearby scout reveal", veil_hides_units_until_scouts_approach},
        {"combat damage and friendly fire", combat_damage_and_friendly_fire_rejection},
        {"save roundtrip and deterministic continuation", save_roundtrip_and_continuation},
        {"corrupt save atomic rejection", corrupted_saves_fail_atomically},
        {"checksummed invalid persistence fields", checksummed_invalid_save_data_is_rejected},
        {"research save invariants and version-1 compatibility", research_saves_reject_maxed_research_and_preserve_valid_states},
        {"research, upgrade and trade progression", research_upgrade_and_trade_progression},
        {"three AI personalities with honest resources", ai_personalities_obey_resource_rules},
        {"AI skirmish ends by castle defeat", normal_skirmish_reaches_castle_defeat},
        {"connected lantern objectives award victory", connected_objectives_award_player_victory},
        {"match outcome and end-state serialization", surrender_closes_commands_and_saves_outcome}
    };
    int passed = 0;
    for (const auto& test : tests) {
        const bool result = test.run();
        std::cout << (result ? "PASS " : "FAIL ") << test.name << '\n';
        if (result) ++passed;
    }
    const int total = static_cast<int>(sizeof(tests) / sizeof(tests[0]));
    std::cout << "Portable simulation tests: " << passed << '/' << total
              << " scenarios, " << assertions << " assertions evaluated.\n"
              << "These tests do not compile Unreal or measure graphics performance.\n";
    return passed == total ? 0 : 1;
}
