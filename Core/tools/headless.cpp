#include "wildlight/Simulation.hpp"
#include <charconv>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
bool number(const char* text, std::uint32_t& value) {
    std::string_view input(text);
    auto result = std::from_chars(input.data(), input.data() + input.size(), value);
    return result.ec == std::errc{} && result.ptr == input.data() + input.size();
}
}

int main(int argc, char** argv) {
    std::uint32_t map = 0, seed = 1, ticks = 600;
    auto personality = wildlight::AIPersonality::Builder;
    auto difficulty = wildlight::Difficulty::Normal;
    std::string save_path, load_path;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--help") {
            std::cout << "Wildlight portable simulation (not Unreal gameplay)\n"
                "--map 0..2 --seed N --ticks N --ai builder|conqueror|arcanist\n"
                "--difficulty easy|normal|hard --save PATH --load PATH\n";
            return 0;
        }
        if (i + 1 >= argc) { std::cerr << "Missing value: " << arg << '\n'; return 2; }
        const std::string value(argv[++i]);
        if (arg == "--map") { if (!number(value.c_str(), map) || map > 2) return 2; }
        else if (arg == "--seed") { if (!number(value.c_str(), seed)) return 2; }
        else if (arg == "--ticks") { if (!number(value.c_str(), ticks) || ticks > 360000) return 2; }
        else if (arg == "--ai") {
            if (value == "builder") personality = wildlight::AIPersonality::Builder;
            else if (value == "conqueror") personality = wildlight::AIPersonality::Conqueror;
            else if (value == "arcanist") personality = wildlight::AIPersonality::Arcanist;
            else return 2;
        } else if (arg == "--difficulty") {
            if (value == "easy") difficulty = wildlight::Difficulty::Easy;
            else if (value == "normal") difficulty = wildlight::Difficulty::Normal;
            else if (value == "hard") difficulty = wildlight::Difficulty::Hard;
            else return 2;
        } else if (arg == "--save") save_path = value;
        else if (arg == "--load") load_path = value;
        else { std::cerr << "Unknown argument: " << arg << '\n'; return 2; }
    }
    wildlight::Simulation simulation;
    simulation.new_match(map, seed, personality, difficulty);
    if (!load_path.empty()) {
        std::ifstream input(load_path, std::ios::binary | std::ios::ate);
        if (!input || input.tellg() > 8 * 1024 * 1024) { std::cerr << "Missing or oversized save.\n"; return 3; }
        input.seekg(0);
        const std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        const auto loaded = simulation.deserialize(bytes);
        if (!loaded.accepted) { std::cerr << "Save rejected: " << loaded.reason << '\n'; return 3; }
    }
    const auto started = std::chrono::steady_clock::now();
    for (std::uint32_t tick = 0; tick < ticks; ++tick) {
        if (simulation.state().outcome != wildlight::MatchOutcome::Playing) break;
        simulation.step(wildlight::FixedStep);
    }
    const double milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    const auto& state = simulation.state();
    std::size_t connected = 0;
    for (const auto& building : state.buildings) if (building.connected && building.health > 0) ++connected;
    std::cout << "{\"engine\":\"portable-core\",\"tick\":" << state.tick
              << ",\"map\":" << state.map_index << ",\"units\":" << state.units.size()
              << ",\"buildings\":" << state.buildings.size() << ",\"light_connected\":" << connected
              << ",\"outcome\":" << static_cast<int>(state.outcome)
              << ",\"simulation_ms\":" << milliseconds << ",\"graphics_measured\":false,\"factions\":[";
    for (std::size_t i = 0; i < state.factions.size(); ++i) {
        if (i) std::cout << ',';
        const auto& faction = state.factions[i];
        std::cout << "{\"wood\":" << faction.resources.wood << ",\"stone\":" << faction.resources.stone
                  << ",\"food\":" << faction.resources.food << ",\"gold\":" << faction.resources.gold
                  << ",\"mana\":" << faction.resources.mana << ",\"population\":" << faction.population
                  << ",\"population_limit\":" << faction.population_limit << ",\"morale\":" << faction.morale << '}';
    }
    std::cout << "]}\n";
    if (!save_path.empty()) {
        std::ofstream output(save_path, std::ios::binary | std::ios::trunc);
        const auto bytes = simulation.serialize();
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!output) { std::cerr << "Could not write save.\n"; return 4; }
    }
    return 0;
}
