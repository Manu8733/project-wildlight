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

// Persistence regression helpers intentionally exercise the file
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
std::string wrap_save(const std::string& raw, std::uint32_t version = SaveVersion) {
    std::uint32_t checksum = 2166136261u;
    for (const unsigned char byte : raw) { checksum ^= byte; checksum *= 16777619u; }
    std::string saved = "WILDLIGHT " + std::to_string(version) + " " + std::to_string(checksum) + "\n";
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
void write_u64(std::string& raw, std::size_t offset, std::uint64_t value) {
    for (int byte = 0; byte < 8; ++byte) raw[offset + byte] = static_cast<char>(value >> (8 * byte));
}
std::size_t growth_extension_offset(const Simulation& sim, const std::string& raw) {
    return raw.size() - (58 + 20 * sim.state().buildings.size());
}
std::string legacy_payload(const Simulation& sim) {
    auto raw = raw_save(sim.serialize());
    // The exact old payload ends before v2's faction count and metadata.
    raw.resize(growth_extension_offset(sim, raw));
    return raw;
}
bool payload_preserved(const Simulation& sim, const std::string& raw, std::uint32_t version) {
    const auto restored = raw_save(sim.serialize());
    return version == 1 ? restored.substr(0, growth_extension_offset(sim, restored)) == raw
                        : restored == raw;
}
void isolate_growth(Simulation& sim) {
    auto& state = const_cast<State&>(sim.state());
    state.ai_timer = 100;
    for (auto& unit : state.units) {
        unit.order = OrderType::Idle;
        unit.target_id = 0;
        unit.job_id = 0;
        unit.path.clear();
        unit.path_index = 0;
        unit.automatic_work = false;
        unit.stance = Stance::Hold;
    }
}
Building& test_building(Simulation& sim, Id id) {
    return *const_cast<Building*>(sim.building(id));
}
Id completed_growth_building(Simulation& sim, BuildingKind kind, Owner owner = Owner::Player) {
    const Id id = place_near_castle(sim, kind, owner);
    if (id) test_building(sim, id).progress = 1;
    isolate_growth(sim);
    return id;
}
void add_residents(Simulation& sim, int count, Owner owner = Owner::Player) {
    auto& state = const_cast<State&>(sim.state());
    const Unit worker = *sim.unit(first_unit(sim, owner, UnitKind::Worker));
    for (int i = 0; i < count; ++i) {
        Unit resident = worker;
        resident.id = state.next_id++;
        state.units.push_back(resident);
    }
}
Command growth_policy(Owner owner = Owner::Player, bool enabled = true,
                      double wood = 100, double stone = 50) {
    Command command;
    command.type = CommandType::SetAutomaticGrowth;
    command.owner = owner;
    command.automatic_growth = {enabled, wood, stone};
    return command;
}

bool automatic_policy_validation_is_atomic() {
    Simulation sim;
    CHECK(sim.state().factions[0].automatic_growth.enabled);
    CHECK(sim.state().factions[1].automatic_growth.enabled);
    CHECK(sim.issue(growth_policy(Owner::Player, false, 125, 75)));
    CHECK(!sim.state().factions[0].automatic_growth.enabled);
    CHECK(sim.state().factions[0].automatic_growth.protected_wood == 125);
    CHECK(sim.state().factions[1].automatic_growth.protected_wood == 100);
    CHECK(sim.issue(growth_policy(Owner::Enemy, true, 1000000, 1000000)));
    CHECK(sim.issue(growth_policy(Owner::Player, true, 0, 0)));
    for (double invalid : {-1.0, 1000000.01, std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()}) {
        CHECK(reject_without_mutation(sim, growth_policy(Owner::Player, true, invalid, 50)));
        CHECK(reject_without_mutation(sim, growth_policy(Owner::Player, true, 100, invalid)));
    }
    CHECK(reject_without_mutation(sim, growth_policy(Owner::Neutral)));
    CHECK(reject_without_mutation(sim, growth_policy(static_cast<Owner>(255))));
    auto malformed = growth_policy(); malformed.target_id = first_building(sim, Owner::Enemy, BuildingKind::Castle);
    CHECK(reject_without_mutation(sim, malformed));
    malformed.target_id = 0; malformed.ids = {first_unit(sim, Owner::Player, UnitKind::Worker)};
    CHECK(reject_without_mutation(sim, malformed));
    return true;
}

bool growth_reserves_use_deposited_stock_and_tolerance() {
    for (double deficit : {0.0, 5e-9, 2e-8}) {
        Simulation sim;
        const Id farm = completed_growth_building(sim, BuildingKind::Farm);
        CHECK(farm);
        test_building(sim, farm).harvested_food = 120;
        auto& stock = const_cast<State&>(sim.state()).factions[0].resources;
        stock.wood = 145.5 - deficit; stock.stone = 56.5;
        advance(sim, 10);
        CHECK(sim.building(farm)->level == (deficit <= 1e-8 ? 2 : 1));
        CHECK(stock.wood + 1e-8 >= (deficit <= 1e-8 ? 100 : 145.5 - deficit));
        CHECK(stock.stone + 1e-8 >= 50);
    }
    Simulation sim;
    const Id farm = completed_growth_building(sim, BuildingKind::Farm);
    CHECK(farm);
    test_building(sim, farm).harvested_food = 120;
    auto& state = const_cast<State&>(sim.state());
    state.factions[0].resources.wood = 145;
    state.factions[0].resources.stone = 56.5;
    for (auto& unit : state.units)
        if (unit.owner == Owner::Player && unit.kind == UnitKind::Worker) {
            unit.cargo_kind = ResourceKind::Wood; unit.cargo = 12;
        }
    advance(sim, 10);
    CHECK(sim.building(farm)->level == 1);
    CHECK(sim.automatic_growth_status(farm).find("reserve") != std::string::npos);
    CHECK(sim.issue(growth_policy(Owner::Player, true, 1000000, 1000000)));
    advance(sim, 10);
    CHECK(sim.building(farm)->level == 1);
    CHECK(state.factions[0].resources.wood == 145);
    for (double stone_deficit : {5e-9, 2e-8}) {
        Simulation boundary;
        const Id boundary_farm = completed_growth_building(boundary, BuildingKind::Farm);
        CHECK(boundary_farm);
        test_building(boundary, boundary_farm).harvested_food = 120;
        CHECK(boundary.issue(growth_policy(Owner::Player, true, 0, 0)));
        auto& stock = const_cast<State&>(boundary.state()).factions[0].resources;
        stock.wood = 45.5; stock.stone = 6.5 - stone_deficit;
        advance(boundary, 10);
        CHECK(boundary.building(boundary_farm)->level == (stone_deficit < 1e-8 ? 2 : 1));
        CHECK(stock.wood >= 0 && stock.stone >= 0);
    }
    return true;
}

bool housing_demand_resets_and_counts_queued_recruits() {
    Simulation sim;
    const Id house = completed_growth_building(sim, BuildingKind::House);
    CHECK(house);
    auto& state = const_cast<State&>(sim.state());
    auto& castle = test_building(sim, first_building(sim, Owner::Player, BuildingKind::Castle));
    castle.train_queue.assign(8, UnitKind::Worker);
    castle.train_timer = 100;
    advance(sim, 99);
    CHECK(state.factions[0].population == 21);
    CHECK(state.factions[0].housing_demand_ticks == 99);
    castle.train_queue.clear();
    advance(sim, 1);
    CHECK(state.factions[0].housing_demand_ticks == 0);
    CHECK(sim.building(house)->level == 1);
    castle.train_queue.assign(8, UnitKind::Worker);
    advance(sim, 50);
    CHECK(state.factions[0].housing_demand_ticks == 50);
    CHECK(sim.issue(growth_policy(Owner::Player, false)));
    CHECK(state.factions[0].housing_demand_ticks == 0);
    advance(sim, 50);
    CHECK(state.factions[0].housing_demand_ticks == 0);
    CHECK(sim.issue(growth_policy()));
    advance(sim, 99);
    CHECK(sim.building(house)->level == 1);
    advance(sim, 1);
    CHECK(sim.building(house)->level == 2);
    CHECK(state.factions[0].population_limit == 34);
    advance(sim, 1);
    CHECK(state.factions[0].housing_demand_ticks == 0);
    return true;
}

bool growth_priority_uses_stable_ids_and_skips_blocked_houses() {
    Simulation sim;
    const Id house = completed_growth_building(sim, BuildingKind::House);
    const Id other_house = completed_growth_building(sim, BuildingKind::House);
    const Id farm = completed_growth_building(sim, BuildingKind::Farm);
    CHECK(house && other_house && farm);
    test_building(sim, farm).harvested_food = 120;
    add_residents(sim, 15);
    auto& state = const_cast<State&>(sim.state());
    state.factions[0].automatic_growth_cooldown_ticks = 100;
    std::reverse(state.buildings.begin(), state.buildings.end());
    advance(sim, 100);
    CHECK(sim.building(house)->level == 2);
    CHECK(sim.building(other_house)->level == 1);
    CHECK(sim.building(farm)->level == 1);
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 150);
    CHECK(state.factions[0].population_limit == 42);
    CHECK(near(state.factions[0].resources.wood, 450 - 2 * 45 - 70 - 29.25));
    Simulation blocked;
    const Id blocked_house = completed_growth_building(blocked, BuildingKind::House);
    const Id ready_farm = completed_growth_building(blocked, BuildingKind::Farm);
    CHECK(blocked_house && ready_farm);
    add_residents(blocked, 8);
    test_building(blocked, blocked_house).health = definition(BuildingKind::House).health * 0.89;
    test_building(blocked, ready_farm).harvested_food = 120;
    const_cast<State&>(blocked.state()).factions[0].automatic_growth_cooldown_ticks = 100;
    advance(blocked, 100);
    CHECK(blocked.building(blocked_house)->level == 1);
    CHECK(blocked.building(ready_farm)->level == 2);
    Simulation unaffordable;
    const Id expensive_house = completed_growth_building(unaffordable, BuildingKind::House);
    const Id cheaper_farm = completed_growth_building(unaffordable, BuildingKind::Farm);
    CHECK(expensive_house && cheaper_farm);
    test_building(unaffordable, expensive_house).level = 2;
    test_building(unaffordable, expensive_house).health = definition(BuildingKind::House).health * 1.35;
    test_building(unaffordable, cheaper_farm).harvested_food = 120;
    add_residents(unaffordable, 15);
    auto& unaffordable_state = const_cast<State&>(unaffordable.state());
    unaffordable_state.factions[0].resources.wood = 150;
    unaffordable_state.factions[0].automatic_growth_cooldown_ticks = 100;
    advance(unaffordable, 100);
    CHECK(unaffordable.building(expensive_house)->level == 2);
    CHECK(unaffordable.building(cheaper_farm)->level == 2);
    CHECK(near(unaffordable_state.factions[0].resources.wood, 104.5));
    return true;
}

bool farm_history_credits_actual_cargo_and_survives_disable() {
    Simulation sim;
    const Id farm = completed_growth_building(sim, BuildingKind::Farm);
    CHECK(farm);
    CHECK(sim.issue(growth_policy(Owner::Player, false)));
    auto& worker = *const_cast<Unit*>(sim.unit(first_unit(sim, Owner::Player, UnitKind::Worker)));
    worker.position = {sim.building(farm)->position.x + 2, sim.building(farm)->position.y};
    worker.order = OrderType::Gather; worker.target_id = farm;
    worker.cargo_kind = ResourceKind::Food; worker.cargo = 11.9;
    const double stock = sim.state().factions[0].resources.food;
    advance(sim, 1);
    CHECK(near(sim.building(farm)->harvested_food, 0.1));
    CHECK(near(worker.cargo, 12));
    CHECK(near(sim.state().factions[0].resources.food, stock));
    test_building(sim, farm).harvested_food = 119.95;
    worker.order = OrderType::Gather; worker.cargo = 0; worker.target_id = farm;
    advance(sim, 1);
    CHECK(sim.building(farm)->harvested_food == 120);
    CHECK(!sim.state().factions[0].automatic_growth.enabled);
    worker.order = OrderType::Idle;
    CHECK(sim.issue(growth_policy()));
    advance(sim, 8);
    CHECK(sim.building(farm)->level == 2);
    CHECK(sim.building(farm)->harvested_food == 0);
    CHECK(sim.issue(growth_policy(Owner::Player, false)));
    worker.order = OrderType::Gather; worker.cargo = 0; worker.target_id = farm;
    advance(sim, 1);
    CHECK(sim.building(farm)->harvested_food > 0.5);
    test_building(sim, farm).harvested_food = 239.99;
    advance(sim, 1);
    CHECK(sim.building(farm)->harvested_food == 240);
    worker.order = OrderType::Idle;
    CHECK(sim.issue(growth_policy()));
    advance(sim, 148);
    CHECK(sim.state().tick == 160);
    CHECK(sim.building(farm)->level == 3);
    CHECK(sim.building(farm)->harvested_food == 0);
    worker.order = OrderType::Gather; worker.cargo = 0; worker.target_id = farm;
    advance(sim, 1);
    CHECK(sim.building(farm)->harvested_food == 0);
    return true;
}

bool manual_growth_bypasses_reserve_and_resets_auto_cooldown() {
    Simulation sim;
    const Id farm = completed_growth_building(sim, BuildingKind::Farm);
    const Id house = completed_growth_building(sim, BuildingKind::House);
    CHECK(farm && house);
    test_building(sim, farm).harvested_food = 120;
    auto& state = const_cast<State&>(sim.state());
    state.factions[0].resources.wood = 120;
    state.factions[0].resources.stone = 20;
    CHECK(sim.issue(growth_policy(Owner::Player, true, 1000000, 1000000)));
    Command upgrade; upgrade.type = CommandType::Upgrade; upgrade.target_id = farm;
    test_building(sim, farm).health = definition(BuildingKind::Farm).health * 0.5;
    CHECK(sim.issue(upgrade));
    CHECK(sim.building(farm)->level == 2);
    CHECK(sim.building(farm)->harvested_food == 0);
    CHECK(near(sim.building(farm)->health, definition(BuildingKind::Farm).health * 1.35 * 0.5));
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 150);
    CHECK(near(state.factions[0].resources.wood, 74.5));
    upgrade.target_id = house;
    advance(sim, 1);
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 149);
    CHECK(sim.issue(upgrade));
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 150);
    upgrade.target_id = farm;
    CHECK(reject_without_mutation(sim, upgrade));
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 150);
    CHECK(sim.issue(growth_policy(Owner::Player, false)));
    sim.step(0);
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 150);
    advance(sim, 150);
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 0);
    return true;
}

bool damage_health_completion_and_maximum_gate_growth() {
    for (int condition = 0; condition < 5; ++condition) {
        Simulation sim;
        const Id farm = completed_growth_building(sim, BuildingKind::Farm);
        CHECK(farm);
        auto& building = test_building(sim, farm);
        building.harvested_food = 120;
        if (condition == 0) building.progress = 0.99;
        if (condition == 1) building.health *= 0.899;
        if (condition == 2) building.last_damage_tick = 1;
        if (condition == 3) { building.level = 3; building.harvested_food = 0; }
        if (condition == 4) building.health *= 0.9;
        advance(sim, 100);
        CHECK(sim.building(farm)->level == (condition == 3 ? 3 : condition == 4 ? 2 : 1));
        if (condition == 2) {
            CHECK(sim.automatic_growth_status(farm).find("damage") != std::string::npos);
            advance(sim, 9);
            CHECK(sim.building(farm)->level == 1);
            advance(sim, 1);
            CHECK(sim.building(farm)->level == 2);
        }
    }
    Simulation sim;
    const Id farm = completed_growth_building(sim, BuildingKind::Farm);
    CHECK(farm);
    test_building(sim, farm).harvested_food = 120;
    auto& state = const_cast<State&>(sim.state());
    const Id projectile = state.next_id++;
    state.projectiles.push_back({projectile, Owner::Enemy, sim.building(farm)->position, farm, 4, 10, false});
    advance(sim, 1);
    CHECK(sim.building(farm)->last_damage_tick == 1);
    CHECK(sim.building(farm)->health < definition(BuildingKind::Farm).health);
    advance(sim, 99);
    CHECK(sim.building(farm)->level == 1);
    advance(sim, 10);
    CHECK(sim.building(farm)->level == 2);
    return true;
}

bool automatic_growth_uses_absolute_phase_and_both_factions() {
    Simulation sim;
    const Id player = completed_growth_building(sim, BuildingKind::Farm);
    const Id enemy = completed_growth_building(sim, BuildingKind::Farm, Owner::Enemy);
    CHECK(player && enemy);
    auto& state = const_cast<State&>(sim.state());
    state.tick = 7;
    test_building(sim, player).harvested_food = 120;
    test_building(sim, enemy).harvested_food = 120;
    advance(sim, 2);
    CHECK(sim.building(player)->level == 1 && sim.building(enemy)->level == 1);
    advance(sim, 1);
    CHECK(sim.building(player)->level == 2 && sim.building(enemy)->level == 2);
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 150);
    CHECK(state.factions[1].automatic_growth_cooldown_ticks == 150);
    test_building(sim, player).harvested_food = 240;
    test_building(sim, enemy).harvested_food = 240;
    advance(sim, 149);
    CHECK(sim.building(player)->level == 2 && sim.building(enemy)->level == 2);
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 1);
    advance(sim, 1);
    CHECK(sim.building(player)->level == 3 && sim.building(enemy)->level == 3);
    Simulation manual_phase;
    const Id manually_grown = completed_growth_building(manual_phase, BuildingKind::Farm);
    CHECK(manually_grown);
    auto& phase = const_cast<State&>(manual_phase.state());
    phase.tick = 7;
    Command upgrade; upgrade.type = CommandType::Upgrade; upgrade.target_id = manually_grown;
    CHECK(manual_phase.issue(upgrade));
    test_building(manual_phase, manually_grown).harvested_food = 240;
    advance(manual_phase, 150);
    CHECK(phase.tick == 157 && phase.factions[0].automatic_growth_cooldown_ticks == 0);
    CHECK(manual_phase.building(manually_grown)->level == 2);
    advance(manual_phase, 3);
    CHECK(manual_phase.building(manually_grown)->level == 3);
    return true;
}

bool terminal_outcomes_prevent_automatic_spending() {
    for (Owner loser : {Owner::Player, Owner::Enemy}) {
        Simulation sim;
        const Id farm = completed_growth_building(sim, BuildingKind::Farm);
        CHECK(farm);
        test_building(sim, farm).harvested_food = 120;
        auto& state = const_cast<State&>(sim.state());
        state.tick = 9;
        test_building(sim, first_building(sim, loser, BuildingKind::Castle)).health = 0;
        state.factions[0].resources.wood = 200;
        const double wood = state.factions[0].resources.wood;
        advance(sim, 1);
        CHECK(state.outcome == (loser == Owner::Player ? MatchOutcome::EnemyVictory : MatchOutcome::PlayerVictory));
        CHECK(sim.building(farm)->level == 1);
        CHECK(state.factions[0].resources.wood == wood);
        CHECK(state.factions[0].housing_demand_ticks == 0);
    }
    Simulation sim;
    const Id farm = completed_growth_building(sim, BuildingKind::Farm);
    CHECK(farm);
    test_building(sim, farm).harvested_food = 120;
    const double wood = sim.state().factions[0].resources.wood;
    Command surrender; surrender.type = CommandType::Surrender;
    CHECK(sim.issue(surrender));
    advance(sim, 10);
    CHECK(sim.building(farm)->level == 1);
    CHECK(sim.state().factions[0].resources.wood == wood);
    return true;
}

bool explicit_purchases_spend_before_automatic_growth() {
    Simulation sim;
    const Id farm = completed_growth_building(sim, BuildingKind::Farm);
    CHECK(farm);
    auto& state = const_cast<State&>(sim.state());
    state.tick = 9;
    state.factions[0].resources.wood = 145.5;
    state.factions[0].resources.stone = 56.5;
    test_building(sim, farm).harvested_food = 120;
    CHECK(sim.automatic_growth_status(farm) == "Ready");
    const Id house = place_near_castle(sim, BuildingKind::House);
    CHECK(house);
    isolate_growth(sim);
    CHECK(near(state.factions[0].resources.wood, 100.5));
    CHECK(near(state.factions[0].resources.stone, 46.5));
    advance(sim, 1);
    CHECK(sim.building(farm)->level == 1);
    CHECK(near(state.factions[0].resources.wood, 100.5));
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 0);
    return true;
}

bool same_tick_manual_upgrades_emit_distinct_level_events() {
    Simulation sim;
    const Id house = completed_growth_building(sim, BuildingKind::House);
    CHECK(house);
    const auto before = sim.state().events.size();
    Command upgrade; upgrade.type = CommandType::Upgrade; upgrade.target_id = house;
    CHECK(near(building_upgrade_cost(*sim.building(house)).wood, 29.25));
    CHECK(sim.issue(upgrade));
    CHECK(near(building_upgrade_cost(*sim.building(house)).wood, 58.5));
    CHECK(sim.issue(upgrade));
    CHECK(sim.state().events.size() == before + 2);
    const auto& second = sim.state().events[before];
    const auto& third = sim.state().events[before + 1];
    CHECK(second.tick == third.tick && second.subject_id == house && third.subject_id == house);
    CHECK(second.text == "Building upgraded to level 2.");
    CHECK(third.text == "Building upgraded to level 3.");
    return true;
}

bool version_one_migration_preserves_legacy_fields_without_inventing_history() {
    Simulation original;
    const Id house = completed_growth_building(original, BuildingKind::House);
    const Id farm = completed_growth_building(original, BuildingKind::Farm);
    CHECK(house && farm);
    add_residents(original, 8);
    CHECK(original.issue(growth_policy(Owner::Player, false)));
    advance(original, 17);
    test_building(original, farm).production_timer = 77;
    test_building(original, farm).harvested_food = 115;
    test_building(original, farm).last_damage_tick = 16;
    const auto raw = legacy_payload(original);
    const auto old_save = wrap_save(raw, 1);
    CHECK(old_save.starts_with("WILDLIGHT 1 "));
    Simulation restored;
    CHECK(restored.deserialize(old_save));
    CHECK(payload_preserved(restored, raw, 1));
    CHECK(restored.serialize().starts_with("WILDLIGHT 2 "));
    for (const auto& faction : restored.state().factions) {
        CHECK(!faction.automatic_growth.enabled);
        CHECK(faction.automatic_growth.protected_wood == 100);
        CHECK(faction.automatic_growth.protected_stone == 50);
        CHECK(faction.housing_demand_ticks == 0);
        CHECK(faction.automatic_growth_cooldown_ticks == 0);
    }
    for (const auto& building : restored.state().buildings) {
        CHECK(building.harvested_food == 0);
        CHECK(building.last_damage_tick == 0);
    }
    CHECK(restored.building(farm)->production_timer == 77);
    advance(restored, 113);
    CHECK(restored.building(house)->level == 1);
    CHECK(restored.issue(growth_policy()));
    advance(restored, 99);
    CHECK(restored.building(house)->level == 1);
    advance(restored, 1);
    CHECK(restored.building(house)->level == 2);
    const auto before = restored.serialize();
    std::vector<std::string> invalid{
        wrap_save(raw.substr(0, raw.size() - 1), 1), wrap_save(raw + "extra", 1),
        wrap_save(raw, 0), wrap_save(raw, 3), wrap_save(raw, 2)
    };
    auto damaged = raw; write_u32(damaged, 16, 0); invalid.push_back(wrap_save(damaged, 1));
    damaged = raw; write_double(damaged, 8, std::numeric_limits<double>::quiet_NaN());
    invalid.push_back(wrap_save(damaged, 1));
    for (const auto& bytes : invalid) {
        CHECK(!restored.deserialize(bytes));
        CHECK(restored.serialize() == before);
    }
    return true;
}

bool version_two_extension_rejects_invalid_metadata_atomically() {
    Simulation live;
    const Id farm = completed_growth_building(live, BuildingKind::Farm);
    const Id second_farm = completed_growth_building(live, BuildingKind::Farm);
    const Id house = completed_growth_building(live, BuildingKind::House);
    CHECK(farm && second_farm && house);
    advance(live, 7);
    test_building(live, farm).harvested_food = 120;
    test_building(live, second_farm).level = 2;
    test_building(live, second_farm).harvested_food = 240;
    const auto before = live.serialize();
    const auto raw = raw_save(before);
    const auto extension = growth_extension_offset(live, raw);
    CHECK(read_u32(raw, extension) == 2);
    CHECK(read_u32(raw, extension + 54) == live.state().buildings.size());
    Simulation valid;
    CHECK(valid.deserialize(before));
    CHECK(valid.serialize() == before);
    std::vector<std::string> invalid;
    const auto wrong_u32 = [&](std::size_t offset, std::uint32_t value) {
        auto changed = raw; write_u32(changed, offset, value); invalid.push_back(wrap_save(changed));
    };
    const auto wrong_double = [&](std::size_t offset, double value) {
        auto changed = raw; write_double(changed, offset, value); invalid.push_back(wrap_save(changed));
    };
    for (std::uint32_t count : {0u, 1u, 3u, 0xffffffffu}) wrong_u32(extension, count);
    for (std::size_t faction = 0; faction < 2; ++faction) {
        const auto base = extension + 4 + 25 * faction;
        auto changed = raw; changed[base] = 2; invalid.push_back(wrap_save(changed));
        changed = raw; changed[base] = 0; write_u32(changed, base + 17, 1);
        invalid.push_back(wrap_save(changed));
        for (double value : {-0.01, 1000000.01, std::numeric_limits<double>::quiet_NaN(),
                             std::numeric_limits<double>::infinity()}) {
            wrong_double(base + 1, value);
            wrong_double(base + 9, value);
        }
        wrong_u32(base + 17, 101);
        wrong_u32(base + 21, 151);
    }
    wrong_u32(extension + 54, 0);
    wrong_u32(extension + 54, static_cast<std::uint32_t>(live.state().buildings.size() - 1));
    wrong_u32(extension + 54, static_cast<std::uint32_t>(live.state().buildings.size() + 1));
    const auto metadata = extension + 58;
    wrong_u32(metadata, live.state().next_id);
    wrong_u32(metadata + 20, read_u32(raw, metadata));
    for (std::size_t i = 0; i < live.state().buildings.size(); ++i) {
        const auto& building = live.state().buildings[i];
        const auto entry = metadata + i * 20;
        auto changed = raw; write_u64(changed, entry + 12, live.state().tick + 1);
        invalid.push_back(wrap_save(changed));
        for (double value : {-0.01, std::numeric_limits<double>::quiet_NaN(),
                             std::numeric_limits<double>::infinity()}) wrong_double(entry + 4, value);
        wrong_double(entry + 4, building.kind == BuildingKind::Farm ?
                     (building.level == 1 ? 120.0001 : 240.0001) : 0.0001);
    }
    auto maxed = raw;
    // The second farm's legacy level byte precedes its metadata; validate zero-only history at L3.
    std::size_t cursor = 43 + read_u32(raw, 39);
    for (int i = 0; i < 2; ++i) cursor += 4 + read_u32(raw, cursor) * 16;
    for (int i = 0; i < 4; ++i) cursor += 4 + read_u32(raw, cursor);
    cursor += 260 + 4;
    for (const auto& building : live.state().buildings) {
        if (building.id == second_farm) maxed[cursor + 39] = 3;
        cursor += 85 + building.train_queue.size();
    }
    invalid.push_back(wrap_save(maxed));
    invalid.push_back(wrap_save(raw.substr(0, extension)));
    invalid.push_back(wrap_save(raw.substr(0, raw.size() - 1)));
    invalid.push_back(wrap_save(raw + "extra"));
    for (const auto& bytes : invalid) {
        const auto result = live.deserialize(bytes);
        CHECK(!result && !result.reason.empty());
        CHECK(live.serialize() == before);
    }
    // Valid reordered entries prove that stable IDs, rather than vector indices, bind the data.
    auto reordered = raw;
    const auto first = raw.substr(metadata, 20), last = raw.substr(raw.size() - 20, 20);
    reordered.replace(metadata, 20, last);
    reordered.replace(raw.size() - 20, 20, first);
    CHECK(valid.deserialize(wrap_save(reordered)));
    CHECK(valid.building(farm)->harvested_food == 120);
    CHECK(valid.building(second_farm)->harvested_food == 240);
    return true;
}

bool growth_save_continuation_preserves_dwell_cooldown_and_split_steps() {
    Simulation original, restored;
    const Id house = completed_growth_building(original, BuildingKind::House);
    const Id farm = completed_growth_building(original, BuildingKind::Farm);
    CHECK(house && farm);
    add_residents(original, 8);
    test_building(original, farm).harvested_food = 120;
    auto& state = const_cast<State&>(original.state());
    state.factions[0].automatic_growth_cooldown_ticks = 100;
    advance(original, 99);
    CHECK(state.factions[0].housing_demand_ticks == 99);
    CHECK(state.factions[0].automatic_growth_cooldown_ticks == 1);
    CHECK(restored.deserialize(original.serialize()));
    CHECK(restored.serialize() == original.serialize());
    original.step(FixedStep); restored.step(FixedStep / 2); restored.step(FixedStep / 2);
    CHECK(original.building(house)->level == 2);
    CHECK(restored.serialize() == original.serialize());
    advance(original, 149); advance(restored, 149);
    CHECK(original.building(farm)->level == 1);
    CHECK(restored.serialize() == original.serialize());
    Simulation boundary;
    CHECK(boundary.deserialize(original.serialize()));
    advance(original, 1); advance(restored, 1); advance(boundary, 1);
    CHECK(original.building(farm)->level == 2);
    CHECK(restored.serialize() == original.serialize());
    CHECK(boundary.serialize() == original.serialize());
    Simulation no_capacity;
    isolate_growth(no_capacity);
    test_building(no_capacity, first_building(no_capacity, Owner::Player, BuildingKind::Castle)).progress = 0;
    advance(no_capacity, 100);
    CHECK(no_capacity.state().outcome == MatchOutcome::Playing);
    CHECK(no_capacity.state().factions[0].population_limit == 0);
    CHECK(no_capacity.state().factions[0].housing_demand_ticks == 0);
    return true;
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

bool housing_pressure_grows_after_one_hundred_ticks() {
    Simulation sim;
    const Id house_id = place_near_castle(sim, BuildingKind::House);
    CHECK(house_id);
    auto& state = const_cast<State&>(sim.state());
    for (auto& building : state.buildings)
        if (building.id == house_id) building.progress = 1;
    // Isolate continuous population demand from AI purchases and worker jobs.
    state.ai_timer = 1000;
    for (auto& unit : state.units) {
        unit.automatic_work = false;
        unit.stance = Stance::Hold;
    }
    const Unit worker = *sim.unit(first_unit(sim, Owner::Player, UnitKind::Worker));
    for (int i = 0; i < 8; ++i) {
        Unit resident = worker;
        resident.id = state.next_id++;
        state.units.push_back(resident);
    }
    advance(sim, 99);
    CHECK(sim.state().factions[0].population == 21);
    CHECK(sim.state().factions[0].population_limit == 26);
    CHECK(sim.state().factions[0].resources.wood >= 129.25);
    CHECK(sim.state().factions[0].resources.stone >= 56.5);
    CHECK(sim.building(house_id)->level == 1);
    advance(sim, 1);
    CHECK(sim.building(house_id)->level == 2);
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
    for (const std::uint32_t version : {1u, 2u}) {
    const auto raw = version == 1 ? legacy_payload(sim) : raw_save(before);
    CHECK(raw.size() > 15000);
    CHECK(version == 1 || wrap_save(raw, version) == before);
    std::vector<std::string> invalid;
    const auto wrong_u32 = [&](std::size_t offset, std::uint32_t value) {
        auto changed = raw; write_u32(changed, offset, value); invalid.push_back(wrap_save(changed, version));
    };
    const auto wrong_double = [&](std::size_t offset, double value) {
        auto changed = raw; write_double(changed, offset, value); invalid.push_back(wrap_save(changed, version));
    };
    wrong_double(8, std::numeric_limits<double>::quiet_NaN()); // Accumulator
    wrong_double(8, -0.01);
    wrong_u32(16, 0); // RNG cannot be zero
    wrong_u32(20, 0); // Next stable identifier
    wrong_u32(24, 3); // Map outside catalog
    wrong_u32(28, 65); // Unsupported map dimensions
    wrong_u32(39, 0xffffffffu); // Oversized terrain vector
    auto changed = raw; changed[36] = static_cast<char>(255); invalid.push_back(wrap_save(changed, version));
    changed = raw; changed[43] = static_cast<char>(255); invalid.push_back(wrap_save(changed, version));

    // Advance over terrain, crossing/objective vectors, and the four fog masks.
    std::size_t cursor = 43 + read_u32(raw, 39);
    for (int i = 0; i < 2; ++i) cursor += 4 + read_u32(raw, cursor) * 16;
    for (int i = 0; i < 4; ++i) cursor += 4 + read_u32(raw, cursor);
    CHECK(cursor + 260 + 4 + 170 < raw.size());
    wrong_double(cursor, std::numeric_limits<double>::infinity()); // Player wood
    wrong_double(cursor, -1); // Resource underflow
    changed = raw; changed[cursor + 109] = static_cast<char>(255); invalid.push_back(wrap_save(changed, version));
    cursor += 260;
    CHECK(read_u32(raw, cursor) == 2);
    cursor += 4;
    wrong_double(cursor + 6, std::numeric_limits<double>::quiet_NaN()); // Building position
    wrong_double(cursor + 23, 1e100); // Building health beyond maximum
    wrong_double(cursor + 31, 2); // Construction progress beyond completion
    wrong_u32(cursor + 85, read_u32(raw, cursor)); // Second castle duplicates first ID
    changed = raw; changed[cursor + 5] = static_cast<char>(255); invalid.push_back(wrap_save(changed, version));
    invalid.push_back(wrap_save(raw + "trailing bytes", version));
    for (const auto& bytes : invalid) {
        const auto result = sim.deserialize(bytes);
        CHECK(!result && !result.reason.empty());
        CHECK(sim.serialize() == before);
    }
    }
    return true;
}

bool research_saves_reject_maxed_research_and_preserve_valid_states() {
    Simulation live;
    advance(live, 7);
    const auto before = live.serialize();
    for (const std::uint32_t version : {1u, 2u}) {
    const auto raw = version == 1 ? legacy_payload(live) : raw_save(before);
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
                const auto result = live.deserialize(wrap_save(invalid, version));
                CHECK(!result && !result.reason.empty());
                CHECK(live.serialize() == before);
            }
            for (std::uint8_t level : {std::uint8_t{0}, std::uint8_t{1}}) {
                for (double timer : {0.0, 35.0}) {
                    auto valid = raw;
                    valid[base + technology_offset + technology] = static_cast<char>(level);
                    valid[base + research_offset] = static_cast<char>(technology);
                    write_double(valid, base + research_timer_offset, timer);
                    const auto saved = wrap_save(valid, version);
                    Simulation restored;
                    CHECK(restored.deserialize(saved));
                    CHECK(payload_preserved(restored, valid, version));
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
    const auto saved = wrap_save(completed, version);
    CHECK(restored.deserialize(saved));
    CHECK(payload_preserved(restored, completed, version));
    restored.step(FixedStep);
    for (const auto& faction : restored.state().factions)
        for (auto level : faction.technologies) CHECK(level == 2);
    }
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
        {"housing pressure grows after 100 fixed ticks", housing_pressure_grows_after_one_hundred_ticks},
        {"automatic policy validation is atomic", automatic_policy_validation_is_atomic},
        {"automatic reserves use deposited stock and affordability tolerance", growth_reserves_use_deposited_stock_and_tolerance},
        {"housing demand resets and counts queued recruits", housing_demand_resets_and_counts_queued_recruits},
        {"automatic priority uses stable IDs and skips blocked houses", growth_priority_uses_stable_ids_and_skips_blocked_houses},
        {"farm history credits actual cargo while growth is disabled", farm_history_credits_actual_cargo_and_survives_disable},
        {"manual growth bypasses reserves and resets automatic cooldown", manual_growth_bypasses_reserve_and_resets_auto_cooldown},
        {"damage, health, completion and maximum level gate growth", damage_health_completion_and_maximum_gate_growth},
        {"automatic growth uses absolute phase for both factions", automatic_growth_uses_absolute_phase_and_both_factions},
        {"terminal outcomes prevent automatic spending", terminal_outcomes_prevent_automatic_spending},
        {"explicit purchases spend before automatic growth", explicit_purchases_spend_before_automatic_growth},
        {"same-tick manual upgrades emit distinct tier events", same_tick_manual_upgrades_emit_distinct_level_events},
        {"true version-1 migration preserves gameplay without fabricated history", version_one_migration_preserves_legacy_fields_without_inventing_history},
        {"version-2 extension rejects malformed metadata atomically", version_two_extension_rejects_invalid_metadata_atomically},
        {"growth save continuation preserves dwell, cooldown and split ticks", growth_save_continuation_preserves_dwell_cooldown_and_split_steps},
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
        {"research save invariants and persistence compatibility", research_saves_reject_maxed_research_and_preserve_valid_states},
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
