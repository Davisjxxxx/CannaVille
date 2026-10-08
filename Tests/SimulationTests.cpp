#include "Simulation/Core/DeterministicRng.hpp"
#include "Simulation/Core/Scenario.hpp"
#include "Simulation/Core/Simulation.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

const std::string smoke_json = R"json({
  "schema_version": 1,
  "scenario_id": "test-smoke",
  "simulation_version": "0.1.0-bootstrap",
  "fixed_timestep_seconds": 900,
  "rooms": [
    {"id": "room-a", "width_m": 4, "depth_m": 4, "cell_size_m": 1}
  ],
  "plants": [
    {"id": "plant-a", "room_id": "room-a", "cultivar_id": "placeholder", "x_m": 0.5, "y_m": 0.5, "z_m": 0}
  ]
})json";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void test_deterministic_repeated_runs() {
    const auto scenario = cannaville::core::Scenario::load_json(smoke_json);
    cannaville::core::Simulation first(scenario, 42);
    cannaville::core::Simulation second(scenario, 42);
    for (int i = 0; i < 8; ++i) {
        first.advance_fixed_step();
        second.advance_fixed_step();
    }
    require(first.serialize_state() == second.serialize_state(), "repeated runs were not identical");
    require(first.csv_row() == second.csv_row(), "repeated CSV rows were not identical");
}

void test_seed_isolation() {
    const auto scenario = cannaville::core::Scenario::load_json(smoke_json);
    cannaville::core::Simulation first(scenario, 41);
    cannaville::core::Simulation second(scenario, 42);
    require(first.serialize_state() != second.serialize_state(), "different simulation seeds were not isolated");

    cannaville::core::DeterministicRng first_rng(41);
    cannaville::core::DeterministicRng second_rng(42);
    require(first_rng.next_u64() != second_rng.next_u64(), "different RNG streams produced the same first value");
}

void test_fixed_timestep_and_offline_reconciliation() {
    const auto scenario = cannaville::core::Scenario::load_json(smoke_json);
    cannaville::core::Simulation simulation(scenario, 1);
    simulation.advance_fixed_step();
    require(simulation.observable_state().clock.steps.value == 1, "fixed step count was incorrect");
    require(simulation.observable_state().clock.elapsed.value == 900.0, "fixed step duration was incorrect");
    const auto result = simulation.reconcile_offline_elapsed(cannaville::units::Seconds{1850.0});
    require(result.advanced.value == 1800.0, "offline reconciliation advanced a partial step");
    require(result.remainder.value == 50.0, "offline reconciliation remainder was incorrect");
    require(simulation.observable_state().clock.steps.value == 3, "offline reconciliation step count was incorrect");
}

void test_serialization_round_trip() {
    const auto scenario = cannaville::core::Scenario::load_json(smoke_json);
    cannaville::core::Simulation original(scenario, 99);
    original.advance_fixed_step();
    original.advance_fixed_step();
    const std::string serialized = original.serialize_state();
    cannaville::core::Simulation restored;
    restored.load_serialized_state(serialized);
    require(restored.serialize_state() == serialized, "serialized state did not round-trip exactly");
}

void test_scenario_validation_and_units() {
    const std::string invalid = R"json({
      "schema_version": 1,
      "scenario_id": "invalid",
      "simulation_version": "0.1.0-bootstrap",
      "fixed_timestep_hours": 1,
      "rooms": [{"id": "room-a", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "plants": []
    })json";
    bool rejected = false;
    try {
        (void)cannaville::core::Scenario::load_json(invalid);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "scenario with unlabeled timestep units was accepted");

    const std::string out_of_bounds = R"json({
      "schema_version": 1,
      "scenario_id": "invalid",
      "simulation_version": "0.1.0-bootstrap",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "room-a", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "plants": [{"id": "plant-a", "room_id": "room-a", "cultivar_id": "placeholder", "x_m": 5, "y_m": 0, "z_m": 0}]
    })json";
    rejected = false;
    try {
        (void)cannaville::core::Scenario::load_json(out_of_bounds);
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "out-of-bounds plant scenario was accepted");
}

void test_version_capture_and_spatial_state() {
    const auto scenario = cannaville::core::Scenario::load_json(smoke_json);
    cannaville::core::Simulation simulation(scenario, 7);
    require(simulation.csv_header().find("simulation_version") != std::string::npos, "CSV header omitted version");
    require(simulation.csv_row().find("0.1.0-bootstrap") != std::string::npos, "CSV row omitted version");
    const auto observable = simulation.observable_state();
    require(observable.rooms.size() == 1, "room state was not initialized");
    require(observable.rooms.front().cells.size() == 16, "spatial cells were not initialized");
    require(observable.plants.front().readiness_label == "not_modeled",
            "bootstrap unexpectedly applied biological behavior");
}

} // namespace

int main() {
    try {
        test_deterministic_repeated_runs();
        test_seed_isolation();
        test_fixed_timestep_and_offline_reconciliation();
        test_serialization_round_trip();
        test_scenario_validation_and_units();
        test_version_capture_and_spatial_state();
        std::cout << "PASS: deterministic repeated runs\n"
                  << "PASS: isolated seeds and explicit RNG streams\n"
                  << "PASS: fixed timestep and offline reconciliation\n"
                  << "PASS: serialization round-trip\n"
                  << "PASS: scenario validation and unit rejection\n"
                  << "PASS: version capture and spatial-cell initialization\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
