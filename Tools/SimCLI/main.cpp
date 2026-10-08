#include "Simulation/Core/Scenario.hpp"
#include "Simulation/Core/Simulation.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

struct Arguments {
    std::string scenario_path;
    std::string duration_text;
    std::string output_path;
    std::uint64_t seed{0};
};

void usage() {
    std::cout << "Usage: cannaville-sim run --scenario FILE --duration 24h --seed N --output FILE\n";
}

std::string next_value(int& index, int argc, char** argv, std::string_view option) {
    if (index + 1 >= argc) throw std::invalid_argument("missing value for " + std::string(option));
    return argv[++index];
}

Arguments parse_arguments(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) != "run") {
        usage();
        throw std::invalid_argument("the only supported command is run");
    }
    Arguments arguments;
    for (int index = 2; index < argc; ++index) {
        const std::string option(argv[index]);
        if (option == "--scenario") {
            arguments.scenario_path = next_value(index, argc, argv, option);
        } else if (option == "--duration") {
            arguments.duration_text = next_value(index, argc, argv, option);
        } else if (option == "--seed") {
            arguments.seed = std::stoull(next_value(index, argc, argv, option));
        } else if (option == "--output") {
            arguments.output_path = next_value(index, argc, argv, option);
        } else if (option == "--help") {
            usage();
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown option: " + option);
        }
    }
    if (arguments.scenario_path.empty() || arguments.duration_text.empty() || arguments.output_path.empty()) {
        usage();
        throw std::invalid_argument("--scenario, --duration, and --output are required");
    }
    return arguments;
}

double parse_duration_seconds(const std::string& text) {
    if (text.size() < 2) throw std::invalid_argument("duration must use a suffix: s, m, h, or d");
    const char suffix = text.back();
    const double magnitude = std::stod(text.substr(0, text.size() - 1));
    if (magnitude < 0.0) throw std::invalid_argument("duration must not be negative");
    switch (suffix) {
    case 's': return magnitude;
    case 'm': return magnitude * 60.0;
    case 'h': return magnitude * 3600.0;
    case 'd': return magnitude * 86400.0;
    default: throw std::invalid_argument("duration suffix must be s, m, h, or d");
    }
}

std::string read_text_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("unable to open scenario: " + path);
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

std::uint64_t fnv1a(std::string_view text) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const unsigned char byte : text) {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Arguments arguments = parse_arguments(argc, argv);
        const cannaville::core::Scenario scenario =
            cannaville::core::Scenario::load_json(read_text_file(arguments.scenario_path));
        cannaville::core::Simulation simulation(scenario, arguments.seed);
        const double duration_seconds = parse_duration_seconds(arguments.duration_text);
        const double timestep = scenario.fixed_timestep.value;
        const auto step_count = static_cast<std::uint64_t>(duration_seconds / timestep + 1e-12);
        const double remainder = duration_seconds - static_cast<double>(step_count) * timestep;

        std::ofstream output(arguments.output_path, std::ios::trunc);
        if (!output) throw std::runtime_error("unable to open output: " + arguments.output_path);
        std::ofstream rz_output(arguments.output_path + ".rootzone.csv", std::ios::trunc);
        output << simulation.csv_header();
        output << simulation.csv_row();
        if (rz_output) {
            rz_output << simulation.root_zone_csv_header();
            rz_output << simulation.root_zone_csv_row();
        }
        for (std::uint64_t step = 0; step < step_count; ++step) {
            simulation.advance_fixed_step();
            output << simulation.csv_row();
            if (rz_output) rz_output << simulation.root_zone_csv_row();
        }
        output.close();
        if (rz_output) rz_output.close();

        const auto observable = simulation.observable_state();
        std::cout << "simulation_version=" << observable.simulation_version << '\n'
                  << "scenario_id=" << observable.scenario_id << '\n'
                  << "seed=" << simulation.seed() << '\n'
                  << "steps=" << observable.clock.steps.value << '\n'
                  << "elapsed_seconds=" << observable.clock.elapsed.value << '\n'
                  << "remainder_seconds=" << remainder << '\n'
                  << "state_hash_fnv1a=" << fnv1a(simulation.serialize_state()) << '\n'
                  << "output=" << arguments.output_path << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
