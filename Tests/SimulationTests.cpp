#include "Simulation/Transpiration/Transpiration.hpp"
#include "Simulation/Core/DeterministicRng.hpp"
#include "Simulation/Core/Scenario.hpp"
#include "Simulation/Core/Simulation.hpp"
#include "Simulation/Environment/VaporPressure.hpp"
#include "Simulation/Lighting/LightingSystem.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
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

void require_near(double actual, double expected, double tolerance, const std::string& message) {
    if (std::abs(actual - expected) > tolerance) {
        throw std::runtime_error(message + ": actual=" + std::to_string(actual) +
                                 " expected=" + std::to_string(expected));
    }
}

const std::string p1a_json = R"json({
  "schema_version": 1,
  "scenario_id": "p1a-test",
  "simulation_version": "0.2.0-p1a",
  "fixed_timestep_seconds": 900,
  "rooms": [{
    "id": "room-a", "width_m": 2, "depth_m": 1, "cell_size_m": 1,
    "cells": [
      {
        "id": "cell-a", "center_x_m": 0.5, "center_y_m": 0.5,
        "environment": {
          "air_temperature_c": 25, "relative_humidity_percent": 50,
          "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450,
          "leaf_temperature_c": null
        },
        "lighting_schedule": [
          {"start_seconds": 0, "end_seconds": 3600, "ppfd_umol_per_m2_s": 100},
          {"start_seconds": 7200, "end_seconds": 10800, "ppfd_umol_per_m2_s": 200}
        ]
      },
      {
        "id": "cell-b", "center_x_m": 1.5, "center_y_m": 0.5,
        "environment": {
          "air_temperature_c": 20, "relative_humidity_percent": 50,
          "atmospheric_pressure_kpa": 100, "co2_umol_per_mol": 800,
          "leaf_temperature_c": 22
        },
        "lighting_schedule": [
          {"start_seconds": 0, "end_seconds": 7200, "ppfd_umol_per_m2_s": 100}
        ]
      }
    ]
  }],
  "plants": []
})json";

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

void test_saturation_vapor_pressure_reference_points() {
    const auto at_zero = cannaville::environment::saturation_vapor_pressure_fao56(
        cannaville::units::Celsius{0.0});
    const auto at_twenty_five = cannaville::environment::saturation_vapor_pressure_fao56(
        cannaville::units::Celsius{25.0});
    require_near(at_zero.value, 0.6108, 1e-10, "FAO-56 saturation pressure at 0 C");
    require_near(at_twenty_five.value, 3.167777717507, 1e-10, "FAO-56 saturation pressure at 25 C");

    bool rejected = false;
    try {
        (void)cannaville::environment::saturation_vapor_pressure_fao56(cannaville::units::Celsius{-0.01});
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "temperature below the registered FAO-56 domain was accepted");
}

void test_vapor_pressure_vpd_and_leaf_semantics() {
    const auto saturation = cannaville::environment::saturation_vapor_pressure_fao56(
        cannaville::units::Celsius{25.0});
    const auto at_zero_rh = cannaville::environment::actual_vapor_pressure_from_relative_humidity(
        saturation, cannaville::units::RelativeHumidityPercent{0.0});
    const auto at_half_rh = cannaville::environment::actual_vapor_pressure_from_relative_humidity(
        saturation, cannaville::units::RelativeHumidityPercent{50.0});
    const auto at_full_rh = cannaville::environment::actual_vapor_pressure_from_relative_humidity(
        saturation, cannaville::units::RelativeHumidityPercent{100.0});
    require_near(at_zero_rh.value, 0.0, 1e-12, "actual vapor pressure at 0 percent RH");
    require_near(at_half_rh.value, 1.583888858753, 1e-10, "actual vapor pressure at 50 percent RH");
    require_near(at_full_rh.value, saturation.value, 1e-12, "actual vapor pressure at 100 percent RH");
    require_near(cannaville::environment::air_vpd(saturation, at_half_rh).value,
                 1.583888858753, 1e-10, "air VPD at 25 C and 50 percent RH");

    bool rejected = false;
    try {
        (void)cannaville::environment::actual_vapor_pressure_from_relative_humidity(
            saturation, cannaville::units::RelativeHumidityPercent{-0.01});
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "RH below zero was accepted");
    rejected = false;
    try {
        (void)cannaville::environment::actual_vapor_pressure_from_relative_humidity(
            saturation, cannaville::units::RelativeHumidityPercent{100.01});
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected, "RH above 100 percent was accepted");

    const auto no_leaf = cannaville::environment::leaf_vpd(
        std::nullopt, at_half_rh);
    require(!no_leaf.has_value(), "missing leaf temperature was silently substituted");
    const auto cooler_leaf = cannaville::environment::leaf_vpd(
        cannaville::units::Celsius{22.0}, at_half_rh);
    require(cooler_leaf.has_value(), "leaf VPD was unavailable with a leaf temperature");
    require(std::abs(cooler_leaf->value - 1.060042333457) < 1e-10,
            "leaf VPD did not differ from air VPD as expected");
}

void test_dli_and_photoperiod_accumulation() {
    using cannaville::lighting::LightingSchedule;
    using cannaville::lighting::LightingScheduleSegment;
    using cannaville::lighting::LightingState;
    using cannaville::units::PPFDMicromolesPerSquareMeterSecond;
    using cannaville::units::Seconds;

    LightingSchedule constant{{LightingScheduleSegment{Seconds{0}, Seconds{86400},
                                                        PPFDMicromolesPerSquareMeterSecond{100}}}};
    LightingState state;
    cannaville::lighting::initialize_state(state, constant, 0.0);
    cannaville::lighting::advance_state(state, constant, 0.0, 43200.0);
    require_near(state.dli.value, 4.32, 1e-12, "constant PPFD DLI");
    require_near(state.accumulated_light_on_duration.value, 43200.0, 1e-12, "light-on duration");
    require_near(state.accumulated_dark_duration.value, 0.0, 1e-12, "constant schedule dark duration");
    require_near(state.photoperiod_duration.value, 86400.0, 1e-12, "photoperiod duration");
    cannaville::lighting::advance_state(state, constant, 43200.0, 43200.0);
    require_near(state.dli.value, 0.0, 1e-12, "DLI was not reset at the simulation-day boundary");

    LightingSchedule variable{{
        LightingScheduleSegment{Seconds{0}, Seconds{3600}, PPFDMicromolesPerSquareMeterSecond{100}},
        LightingScheduleSegment{Seconds{3600}, Seconds{7200}, PPFDMicromolesPerSquareMeterSecond{200}},
    }};
    state = LightingState{};
    cannaville::lighting::initialize_state(state, variable, 0.0);
    cannaville::lighting::advance_state(state, variable, 0.0, 7200.0);
    require_near(state.dli.value, 1.08, 1e-12, "variable PPFD DLI");

    LightingSchedule dark_gap{{LightingScheduleSegment{Seconds{0}, Seconds{3600},
                                                         PPFDMicromolesPerSquareMeterSecond{100}}}};
    state = LightingState{};
    cannaville::lighting::initialize_state(state, dark_gap, 0.0);
    cannaville::lighting::advance_state(state, dark_gap, 0.0, 7200.0);
    require_near(state.dli.value, 0.36, 1e-12, "dark-gap DLI");
    require_near(state.accumulated_light_on_duration.value, 3600.0, 1e-12, "dark-gap light duration");
    require_near(state.accumulated_dark_duration.value, 3600.0, 1e-12, "dark-gap dark duration");

    LightingSchedule zero_light{{LightingScheduleSegment{Seconds{0}, Seconds{86400},
                                                          PPFDMicromolesPerSquareMeterSecond{0}}}};
    state = LightingState{};
    cannaville::lighting::initialize_state(state, zero_light, 0.0);
    cannaville::lighting::advance_state(state, zero_light, 0.0, 3600.0);
    require_near(state.dli.value, 0.0, 1e-12, "zero-PPFD DLI");
    require(!state.light_on, "zero-PPFD schedule was marked light-on");

    LightingState one_interval;
    LightingState four_intervals;
    cannaville::lighting::initialize_state(one_interval, constant, 0.0);
    cannaville::lighting::initialize_state(four_intervals, constant, 0.0);
    cannaville::lighting::advance_state(one_interval, constant, 0.0, 14400.0);
    for (int i = 0; i < 4; ++i) {
        cannaville::lighting::advance_state(four_intervals, constant, static_cast<double>(i) * 3600.0, 3600.0);
    }
    require_near(one_interval.dli.value, four_intervals.dli.value, 1e-12,
                 "DLI changed with fixed-step partitioning");
}

void test_sub_timestep_boundaries() {
    using cannaville::lighting::LightingSchedule;
    using cannaville::lighting::LightingScheduleSegment;
    using cannaville::lighting::LightingState;
    using cannaville::units::PPFDMicromolesPerSquareMeterSecond;
    using cannaville::units::Seconds;

    LightingSchedule variable{{
        LightingScheduleSegment{Seconds{1000}, Seconds{2000}, PPFDMicromolesPerSquareMeterSecond{100}},
        LightingScheduleSegment{Seconds{2000}, Seconds{3000}, PPFDMicromolesPerSquareMeterSecond{200}},
    }};

    // light-on transition inside timestep
    LightingState state;
    cannaville::lighting::initialize_state(state, variable, 0.0);
    cannaville::lighting::advance_state(state, variable, 500.0, 1000.0);
    require_near(state.dli.value, (100 * 500) / 1e6, 1e-12, "light-on transition inside timestep DLI");
    require_near(state.accumulated_light_on_duration.value, 500.0, 1e-12, "light-on transition duration");
    require_near(state.photoperiod_duration.value, 2000.0, 1e-12, "photoperiod duration inside timestep");

    // PPFD value change inside timestep
    state = LightingState{};
    cannaville::lighting::initialize_state(state, variable, 1500.0);
    cannaville::lighting::advance_state(state, variable, 1500.0, 1000.0);
    require_near(state.dli.value, (100 * 500 + 200 * 500) / 1e6, 1e-12, "PPFD value change inside timestep DLI");

    // light-off transition inside timestep
    state = LightingState{};
    cannaville::lighting::initialize_state(state, variable, 2500.0);
    cannaville::lighting::advance_state(state, variable, 2500.0, 1000.0);
    require_near(state.dli.value, (200 * 500) / 1e6, 1e-12, "light-off transition inside timestep DLI");
    require_near(state.accumulated_dark_duration.value, 500.0, 1e-12, "light-off dark duration inside timestep");

    // day rollover inside timestep
    state = LightingState{};
    cannaville::lighting::initialize_state(state, variable, 86000.0);
    cannaville::lighting::advance_state(state, variable, 86000.0, 1500.0);
    require_near(state.dli.value, (100 * 100) / 1e6, 1e-12, "day rollover inside timestep DLI");
}

#include "Simulation/GasExchange/GasExchange.hpp"

void test_gas_exchange() {
    using namespace cannaville::gasexchange;
    using namespace cannaville::units;
    
    GasExchangeState state;
    auto tang_profile = get_synthetic_vegetative_test_profile();
    auto med_profile = get_synthetic_high_capacity_test_profile();
    auto unconfigured = get_unconfigured_profile();

    // 1. Missing calibration profile
    solve_coupled_gas_exchange(state, unconfigured, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{1.5}, Celsius{25}, AtmosphericPressureKPa{101.325});
    require(state.status == ConvergenceStatus::MissingCalibrationProfile, "Failed to reject unconfigured profile");

    // 2. Missing leaf temperature
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{1.5}, std::nullopt, AtmosphericPressureKPa{101.325});
    require(state.status == ConvergenceStatus::MissingLeafTemperature, "Failed to handle missing leaf temperature");

    // 3. Zero/near-zero light behavior
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{0}, CO2MicromolesPerMole{400}, VPDKPa{1.5}, Celsius{25}, AtmosphericPressureKPa{101.325});
    require(state.status == ConvergenceStatus::Converged, "Zero light failed to converge");
    require(state.net_assimilation.value < 0.0, "Zero light assimilation must be negative (respiration)");
    require_near(state.stomatal_conductance.value, tang_profile.medlyn.g0, 1e-5, "Zero light conductance should approach g0");

    // 4. Increasing PPFD response
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{100}, CO2MicromolesPerMole{400}, VPDKPa{1.5}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double an_low_light = state.net_assimilation.value;
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{1.5}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double an_high_light = state.net_assimilation.value;
    require(an_high_light > an_low_light, "Assimilation did not increase with PPFD");

    // 5. Changing ambient CO2
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{800}, VPDKPa{1.5}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double an_high_co2 = state.net_assimilation.value;
    require(an_high_co2 > an_high_light, "Assimilation did not increase with CO2");

    // 6. Changing VPD
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{3.0}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double gsw_high_vpd = state.stomatal_conductance.value;
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{1.0}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double gsw_low_vpd = state.stomatal_conductance.value;
    require(gsw_high_vpd < gsw_low_vpd, "Conductance did not decrease with higher VPD");

    // 7. Changing leaf temperature
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{1.5}, Celsius{15}, AtmosphericPressureKPa{101.325});
    double an_low_temp = state.net_assimilation.value;
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{1.5}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double an_opt_temp = state.net_assimilation.value;
    require(an_opt_temp > an_low_temp, "Assimilation did not increase at optimal temperature compared to low temp");

    // 8. Two different calibration profiles
    solve_coupled_gas_exchange(state, med_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{1.5}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double an_med = state.net_assimilation.value;
    require(an_med != an_opt_temp, "Two different calibration profiles yielded identical assimilation");

    // 9. Negative VPD behavior
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{-1.5}, Celsius{25}, AtmosphericPressureKPa{101.325});
    require(state.status == ConvergenceStatus::NegativeVPD, "Failed to handle negative VPD");
}

void test_spatial_physics_and_cli_fields() {
    const auto scenario = cannaville::core::Scenario::load_json(p1a_json);
    cannaville::core::Simulation simulation(scenario, 5);
    const auto& full = simulation.full_state_for_internal_use();
    require(full.rooms.size() == 1 && full.rooms.front().cells.size() == 2,
            "P1A scenario did not initialize two cells");
    require(full.rooms.front().cells[0].environment.air_temperature.value !=
                full.rooms.front().cells[1].environment.air_temperature.value,
            "cell temperatures were not spatially distinct");
    require(full.rooms.front().cells[0].environment.leaf_vpd == std::nullopt,
            "missing leaf temperature produced a leaf VPD");
    require(full.rooms.front().cells[1].environment.leaf_vpd.has_value(),
            "configured leaf temperature did not produce leaf VPD");
    simulation.advance_fixed_step();
    const std::string row = simulation.csv_row();
    require(row.find("cell-a") != std::string::npos && row.find("cell-b") != std::string::npos,
            "CSV did not expose both spatial cells");
    require(row.find("saturation_vapor_pressure_kpa") == std::string::npos,
            "CSV data row unexpectedly contained a header field");
    require(simulation.csv_header().find("leaf_vpd_kpa") != std::string::npos,
            "CSV header omitted leaf VPD");
    require(simulation.csv_header().find("dli_mol_per_m2_day") != std::string::npos,
            "CSV header omitted DLI");

    const std::string serialized = simulation.serialize_state();
    cannaville::core::Simulation restored;
    restored.load_serialized_state(serialized);
    require(restored.serialize_state() == serialized, "P1A spatial state did not round-trip exactly");
}

void test_gas_exchange_robustness_and_domain_guards() {
    using namespace cannaville::gasexchange;
    using namespace cannaville::units;
    
    GasExchangeState state;
    auto tang_profile = get_synthetic_vegetative_test_profile();

    // Test Near-Zero VPD (kMedlynMinimumVPDKPa)
    // VPD of exactly 0.0 or 0.01 should behave exactly as kMedlynMinimumVPDKPa
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{0.0}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double gsw_zero_vpd = state.stomatal_conductance.value;
    
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{0.01}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double gsw_micro_vpd = state.stomatal_conductance.value;
    
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{1000}, CO2MicromolesPerMole{400}, VPDKPa{kMedlynMinimumVPDKPa}, Celsius{25}, AtmosphericPressureKPa{101.325});
    double gsw_floor_vpd = state.stomatal_conductance.value;

    require_near(gsw_zero_vpd, gsw_floor_vpd, 1e-6, "Conductance at 0 VPD should be clamped to kMedlynMinimumVPDKPa floor");
    require_near(gsw_micro_vpd, gsw_floor_vpd, 1e-6, "Conductance at 0.01 VPD should be clamped to kMedlynMinimumVPDKPa floor");

    // Test Darkness/Negative Assimilation bounding
    // If PPFD is 0, An will be negative, and gsw should be exactly g0 because A_for_medlyn is max(0, An)
    solve_coupled_gas_exchange(state, tang_profile, PPFDMicromolesPerSquareMeterSecond{0}, CO2MicromolesPerMole{400}, VPDKPa{1.5}, Celsius{25}, AtmosphericPressureKPa{101.325});
    require(state.net_assimilation.value < 0.0, "Dark assimilation must be negative (respiration only)");
    require_near(state.stomatal_conductance.value, tang_profile.medlyn.g0, 1e-12, "Conductance in darkness (An < 0) must exactly equal g0");

    // Robustness Sweep
    // Sweep through various conditions including extremes, checking for NaNs and silent fallbacks
    std::vector<double> ppfds = {0.0, 10.0, 100.0, 2000.0, 5000.0};
    std::vector<double> co2s = {50.0, 400.0, 2000.0, 10000.0};
    std::vector<double> temps = {5.0, 15.0, 25.0, 40.0, 60.0};
    std::vector<double> vpds = {0.0, 0.5, 2.0, 10.0};

    for (double ppfd : ppfds) {
        for (double co2 : co2s) {
            for (double temp : temps) {
                for (double vpd : vpds) {
                    GasExchangeState sweep_state;
                    solve_coupled_gas_exchange(sweep_state, tang_profile, PPFDMicromolesPerSquareMeterSecond{ppfd}, CO2MicromolesPerMole{co2}, VPDKPa{vpd}, Celsius{temp}, AtmosphericPressureKPa{101.325});
                    
                    if (sweep_state.status == ConvergenceStatus::Converged) {
                        require(!std::isnan(sweep_state.net_assimilation.value), "Assimilation became NaN during robustness sweep");
                        require(!std::isnan(sweep_state.stomatal_conductance.value), "Conductance became NaN during robustness sweep");
                        require(!std::isnan(sweep_state.intercellular_co2.value), "Intercellular CO2 became NaN during robustness sweep");
                        
                        require(sweep_state.stomatal_conductance.value >= tang_profile.medlyn.g0 - 1e-9, "Conductance fell below g0 during sweep");
                        require(sweep_state.intercellular_co2.value >= 0.0, "Intercellular CO2 fell below zero");
                    } else if (sweep_state.status == ConvergenceStatus::FailedToConverge) {
                        require_near(sweep_state.net_assimilation.value, 0.0, 1e-12, "Failed convergence state should not output non-zero values silently");
                    } else if (sweep_state.status == ConvergenceStatus::NegativeVPD) {
                        require(vpd < 0.0, "NegativeVPD returned for non-negative VPD");
                    } else {
                        require(false, "Unexpected status during robustness sweep");
                    }
                }
            }
        }
    }
}

void test_transpiration() {
    using namespace cannaville::transpiration;

    // 1. Air Properties
    auto props = calculate_air_properties(cannaville::units::Celsius{20.0}, cannaville::units::AtmosphericPressureKPa{101.325});
    require_near(props.molar_density, 41.6, 1.0, "Molar density calculation failed");
    require_near(props.kinematic_viscosity, 1.5e-5, 1e-6, "Kinematic viscosity failed");

    // 2. Boundary Layer Conductance (Forced Convection)
    auto gb_forced = calculate_boundary_layer_conductance(cannaville::units::AirflowMetersPerSecond{1.0}, cannaville::units::Meters{0.05}, props, cannaville::units::Celsius{20.0}, cannaville::units::Celsius{20.0});
    require(gb_forced.status == ScientificDomainStatus::Valid, "Forced convection gb failed status");
    require(gb_forced.value_mol_m2_s.value() > 0.0, "Forced convection gb failed value");
    require(gb_forced.regime == "forced_convection_dominant", "Regime should be forced_convection_dominant");

    auto gb_faster = calculate_boundary_layer_conductance(cannaville::units::AirflowMetersPerSecond{2.0}, cannaville::units::Meters{0.05}, props, cannaville::units::Celsius{20.0}, cannaville::units::Celsius{20.0});
    require(gb_faster.value_mol_m2_s.value() > gb_forced.value_mol_m2_s.value(), "Higher wind should increase gb");

    auto gb_larger = calculate_boundary_layer_conductance(cannaville::units::AirflowMetersPerSecond{1.0}, cannaville::units::Meters{0.10}, props, cannaville::units::Celsius{20.0}, cannaville::units::Celsius{20.0});
    require(gb_larger.value_mol_m2_s.value() < gb_forced.value_mol_m2_s.value(), "Larger leaf should decrease gb");

    // 3. Boundary Layer Conductance (Free Convection)
    auto gb_free = calculate_boundary_layer_conductance(cannaville::units::AirflowMetersPerSecond{0.0}, cannaville::units::Meters{0.05}, props, cannaville::units::Celsius{25.0}, cannaville::units::Celsius{20.0});
    require(gb_free.value_mol_m2_s.value() > 0.0, "Free convection gb failed");
    require(gb_free.regime == "free_convection_dominant", "Regime should be free_convection_dominant");

    // 4. Invalid dimension
    auto gb_invalid = calculate_boundary_layer_conductance(cannaville::units::AirflowMetersPerSecond{1.0}, cannaville::units::Meters{-0.05}, props, cannaville::units::Celsius{20.0}, cannaville::units::Celsius{20.0});
    require(gb_invalid.status == ScientificDomainStatus::InvalidDimension, "Invalid dimension not caught");
    require(gb_invalid.regime == "unsupported", "Invalid dimension regime wrong");
    
    // 4b. Turbulent flow
    auto gb_turbulent = calculate_boundary_layer_conductance(cannaville::units::AirflowMetersPerSecond{100.0}, cannaville::units::Meters{1.0}, props, cannaville::units::Celsius{20.0}, cannaville::units::Celsius{20.0});
    require(gb_turbulent.status == ScientificDomainStatus::UnsupportedFlowRegime, "Turbulent flow not caught");
    require(gb_turbulent.regime == "turbulent_unsupported", "Turbulent flow regime wrong");
    require(!gb_turbulent.value_mol_m2_s.has_value(), "Turbulent flow should not return a valid conductance");

    // 5. Transpiration Calculation
    auto trans = calculate_transpiration(cannaville::units::StomatalConductanceMolesPerSquareMeterSecond{0.1}, gb_forced, cannaville::units::VPDKPa{1.5}, cannaville::units::AtmosphericPressureKPa{101.325});
    require(trans.flux_mol_m2_s > 0.0, "Transpiration flux failed");
    require(trans.total_conductance_mol_m2_s < 0.1 && trans.total_conductance_mol_m2_s < gb_forced.value_mol_m2_s.value(), "Series conductance must be less than individual components");

    // 6. Condensation (Negative VPD)
    auto trans_cond = calculate_transpiration(cannaville::units::StomatalConductanceMolesPerSquareMeterSecond{0.1}, gb_forced, cannaville::units::VPDKPa{-0.5}, cannaville::units::AtmosphericPressureKPa{101.325});
    require(trans_cond.status == "condensation", "Condensation not caught");
    require(trans_cond.flux_mol_m2_s < 0.0, "Condensation flux must be negative");

    // 7. Zero Gradient
    auto trans_zero = calculate_transpiration(cannaville::units::StomatalConductanceMolesPerSquareMeterSecond{0.1}, gb_forced, cannaville::units::VPDKPa{0.0}, cannaville::units::AtmosphericPressureKPa{101.325});
    require_near(trans_zero.flux_mol_m2_s, 0.0, 1e-12, "Zero VPD should give zero flux");
}

void test_transpiration_robustness_sweep() {
    using namespace cannaville::transpiration;
    
    std::vector<double> us = {0.0, 0.01, 1.0, 10.0, 100.0};
    std::vector<double> ds = {0.01, 0.1, 1.0};
    std::vector<double> ta = {10.0, 25.0, 40.0};
    std::vector<double> tl = {10.0, 25.0, 45.0};
    std::vector<double> ps = {50.0, 101.325};
    std::vector<double> gs = {0.0, 0.01, 0.5};
    std::vector<double> vpd = {-1.0, 0.0, 2.0};

    int numerically_valid = 0;
    int scientifically_unsupported = 0;
    int numerically_invalid = 0;

    for (double u : us) {
        for (double d : ds) {
            for (double t_a : ta) {
                for (double t_l : tl) {
                    for (double p : ps) {
                        for (double g : gs) {
                            for (double v : vpd) {
                                auto props = calculate_air_properties(cannaville::units::Celsius{t_a}, cannaville::units::AtmosphericPressureKPa{p});
                                auto gb = calculate_boundary_layer_conductance(cannaville::units::AirflowMetersPerSecond{u}, cannaville::units::Meters{d}, props, cannaville::units::Celsius{t_l}, cannaville::units::Celsius{t_a});
                                auto trans = calculate_transpiration(cannaville::units::StomatalConductanceMolesPerSquareMeterSecond{g}, gb, cannaville::units::VPDKPa{v}, cannaville::units::AtmosphericPressureKPa{p});
                                
                                if (gb.status == ScientificDomainStatus::Valid) {
                                    if (std::isnan(gb.value_mol_m2_s.value()) || std::isinf(gb.value_mol_m2_s.value())) {
                                        numerically_invalid++;
                                    } else {
                                        numerically_valid++;
                                    }
                                } else if (gb.status == ScientificDomainStatus::UnsupportedFlowRegime || gb.status == ScientificDomainStatus::InvalidDimension) {
                                    scientifically_unsupported++;
                                } else {
                                    numerically_invalid++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "Robustness sweep counts:\n";
    std::cout << "  NUMERICALLY_VALID: " << numerically_valid << "\n";
    std::cout << "  SCIENTIFICALLY_UNSUPPORTED: " << scientifically_unsupported << "\n";
    std::cout << "  NUMERICALLY_INVALID: " << numerically_invalid << "\n";
    
    require(numerically_invalid == 0, "Encountered numerically invalid states during sweep");
    require(scientifically_unsupported > 0, "Did not encounter expected unsupported scientific domains (e.g., turbulence)");
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
        test_saturation_vapor_pressure_reference_points();
        test_vapor_pressure_vpd_and_leaf_semantics();
        test_dli_and_photoperiod_accumulation();
        test_sub_timestep_boundaries();
        test_spatial_physics_and_cli_fields();
        test_gas_exchange();
        test_gas_exchange_robustness_and_domain_guards();
        test_transpiration();
        test_transpiration_robustness_sweep();
        std::cout << "PASS: deterministic repeated runs\n"
                  << "PASS: isolated seeds and explicit RNG streams\n"
                  << "PASS: fixed timestep and offline reconciliation\n"
                  << "PASS: serialization round-trip\n"
                  << "PASS: scenario validation and unit rejection\n"
                  << "PASS: version capture and spatial-cell initialization\n"
                  << "PASS: saturation vapor pressure reference points\n"
                  << "PASS: vapor pressure, air VPD, and leaf VPD semantics\n"
                  << "PASS: DLI, photoperiod, and dark-interval accumulation\n"
                  << "PASS: sub-timestep boundary integration\n"
                  << "PASS: spatial physics and CSV inspection fields\n"
                  << "PASS: gas exchange models\n"
                  << "PASS: gas exchange robustness and domain guards\n"
                  << "PASS: transpiration\n"
                  << "PASS: transpiration robustness sweep\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
