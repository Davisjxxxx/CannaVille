#include "Simulation/Transpiration/Transpiration.hpp"
#include "Simulation/RootZone/HydraulicLimitation.hpp"
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
    "root_zones": [
    {"id": "rootzone_plant-a", "type": "Substrate", "substrate_bulk_volume_m3": 0.01, "initial_water_volume_m3": 0.005}
  ],
  "plants": [
    {"id": "plant-a", "room_id": "room-a", "cultivar_id": "placeholder", "root_zone_id": "rootzone_plant-a", "x_m": 0.5, "y_m": 0.5, "z_m": 0}
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

#include "Simulation/RootZone/RootZoneWater.hpp"

void test_rootzone_exact_event_timing() {
    using namespace cannaville::rootzone;
    using namespace cannaville::units;
    
    // Simulate 15 minute timestep (900 seconds)
    // - 08:00 (0s) - start
    // - 08:07 (420s) - irrigate 0.005
    // - 08:12 (720s) - irrigate 0.005 + top-off 0.001
    // - 08:14 (840s) - exact boundary drainage event
    // - 08:15 (900s) - end exactly on boundary
    SubstrateContainer container{VolumeCubicMeters{0.05}, VolumeCubicMeters{0.01}, VolumeCubicMeters{0.05}};
    
    WaterFluxMolesPerSquareMeterSecond flux{0.01};
    AreaSquareMeters area{1.0};
    
    // Step 1: 0 to 420 (08:07)
    container.withdraw_transpiration(flux, area, Seconds{420});
    container.irrigate(VolumeCubicMeters{0.005});
    
    // Step 2: 420 to 720 (08:12)
    container.withdraw_transpiration(flux, area, Seconds{300});
    container.irrigate(VolumeCubicMeters{0.005});
    container.add_top_off(VolumeCubicMeters{0.001}); // multiple event types
    
    // Step 3: 720 to 840 (08:14)
    container.withdraw_transpiration(flux, area, Seconds{120});
    container.process_drainage(VolumeCubicMeters{0.002}); // drainage inside timestep
    
    // Step 4: 840 to 900 (08:15)
    container.withdraw_transpiration(flux, area, Seconds{60});
    
    // Verify cumulative
    double total_requested = (420 + 300 + 120 + 60) * flux.value * area.value * moles_water_to_cubic_meters(1.0);
    double expected_final = 0.01 + 0.005 + 0.005 + 0.001 - 0.002 - total_requested;
    
    require_near(container.get_last_balance().plant_withdrawal_requested_m3, total_requested, 1e-10, "Withdrawal total correct");
    require_near(container.current_water().value, expected_final, 1e-10, "Final water correct");
    require(std::abs(container.get_last_balance().residual_m3) < 1e-10, "Conservation residual explicitly bounded < 1e-10");
    
    // Deterministic repeat
    SubstrateContainer container2{VolumeCubicMeters{0.05}, VolumeCubicMeters{0.01}, VolumeCubicMeters{0.05}};
    container2.withdraw_transpiration(flux, area, Seconds{420});
    container2.irrigate(VolumeCubicMeters{0.005});
    container2.withdraw_transpiration(flux, area, Seconds{300});
    container2.irrigate(VolumeCubicMeters{0.005});
    container2.add_top_off(VolumeCubicMeters{0.001});
    container2.withdraw_transpiration(flux, area, Seconds{120});
    container2.process_drainage(VolumeCubicMeters{0.002});
    container2.withdraw_transpiration(flux, area, Seconds{60});
    
    require_near(container.current_water().value, container2.current_water().value, 1e-12, "Repeat run must be deterministic");
}
void test_rootzone_dry_down() {
    using namespace cannaville::rootzone;
    using namespace cannaville::units;
    
    // bulk_volume, initial_water, max_stored_water
    SubstrateContainer container{VolumeCubicMeters{0.01}, VolumeCubicMeters{0.005}, VolumeCubicMeters{0.01}};
    require_near(container.volumetric_water_content(), 0.5, 1e-6, "Initial VWC should be 0.5");
    require_near(container.storage_fraction().value(), 0.5, 1e-6, "Initial storage fraction should be 0.5");
    
    // Withdrawing water
    WaterFluxMolesPerSquareMeterSecond flux{0.005};
    AreaSquareMeters area{1.0};
    Seconds dt{3600};
    
    auto withdrawn = container.withdraw_transpiration(flux, area, dt);
    require(withdrawn.value > 0.0, "Should have withdrawn water");
    require(std::abs(container.get_last_balance().residual_m3) < 1e-12, "Residual should be near zero");
    
    // Dry down to insufficient
    flux.value = 1.0;
    withdrawn = container.withdraw_transpiration(flux, area, dt);
    require(container.get_last_balance().status == WaterStatus::InsufficientRootzoneWater, "Should be insufficient");
    require_near(container.current_water().value, 0.0, 1e-6, "Should be empty");
    require(container.get_last_balance().plant_withdrawal_unmet_m3 > 0.0, "Should have unmet demand");
}

void test_rootzone_pulsed_irrigation() {
    using namespace cannaville::rootzone;
    using namespace cannaville::units;
    
    SubstrateContainer container{VolumeCubicMeters{0.02}, VolumeCubicMeters{0.005}, VolumeCubicMeters{0.01}};
    
    container.irrigate(VolumeCubicMeters{0.002});
    require_near(container.current_water().value, 0.007, 1e-6, "Should have 0.007 m3");
    
    // Oversaturate
    container.irrigate(VolumeCubicMeters{0.005});
    container.process_drainage();
    require_near(container.current_water().value, 0.01, 1e-6, "Should be capped at max capacity");
    require(container.get_last_balance().status == WaterStatus::Overflow, "Should have overflow status");
    require(container.get_last_balance().drainage_removed_m3 > 0.0, "Should have drainage");
    require(std::abs(container.get_last_balance().residual_m3) < 1e-12, "Residual should be near zero");
    require_near(container.volumetric_water_content(), 0.5, 1e-6, "VWC is 0.01/0.02 = 0.5");
}

void test_rootzone_dwc_reservoir() {
    using namespace cannaville::rootzone;
    using namespace cannaville::units;
    
    HydroponicReservoir res{VolumeCubicMeters{0.1}, VolumeCubicMeters{0.08}};
    res.add_external_return_flow(VolumeCubicMeters{0.03});
    res.process_discharge();
    require_near(res.current_water().value, 0.1, 1e-6, "Should be full");
    require(std::abs(res.get_last_balance().residual_m3) < 1e-12, "Residual should be near zero");
    
    auto withdrawn = res.withdraw_transpiration(WaterFluxMolesPerSquareMeterSecond{0.01}, AreaSquareMeters{2.0}, Seconds{3600});
    require(withdrawn.value > 0.0, "Should withdraw water");
    require(res.current_water().value < 0.1, "Should have less water");
    require(std::abs(res.get_last_balance().residual_m3) < 1e-12, "Residual should be near zero");
}

void test_rootzone_shared_allocation() {
    using namespace cannaville::rootzone;
    using namespace cannaville::units;
    
    SubstrateContainer container{VolumeCubicMeters{0.02}, VolumeCubicMeters{0.01}};
    
    std::vector<TranspirationRequest> requests = {
        {WaterFluxMolesPerSquareMeterSecond{10.0}, AreaSquareMeters{1.0}},
        {WaterFluxMolesPerSquareMeterSecond{30.0}, AreaSquareMeters{1.0}}
    };
    
    auto realized = container.withdraw_transpiration_shared(requests, Seconds{3600});
    require(container.get_last_balance().status == WaterStatus::InsufficientRootzoneWater, "Should be insufficient");
    require_near(container.current_water().value, 0.0, 1e-6, "Should be empty");
    
    // Proportional allocation: 1:3 ratio
    require_near(realized[0].value, 0.0025, 1e-6, "25% of water");
    require_near(realized[1].value, 0.0075, 1e-6, "75% of water");
    require(std::abs(container.get_last_balance().residual_m3) < 1e-12, "Residual should be near zero");
}

void test_rootzone_exact_event_timing();
void test_rootzone_unit_conversion() {
    using namespace cannaville::rootzone;
    double moles = 1000.0;
    double m3 = moles_water_to_cubic_meters(moles);
    // 1000 moles * 18.01528 g/mol / 998.2 kg/m^3 = 0.01804776 m3
    require_near(m3, 0.01804776, 1e-6, "Molar conversion check");
}

void test_rootzone_robustness_sweep() {
    using namespace cannaville::rootzone;
    using namespace cannaville::units;
    
    std::vector<double> init_water = {0.0, 0.005, 0.01, 0.02};
    std::vector<double> irrigations = {0.0, 0.001, 0.02};
    std::vector<double> fluxes = {0.0, 0.005, 0.1};
    
    int numerically_invalid = 0;
    
    for (double iw : init_water) {
        for (double irr : irrigations) {
            for (double flux : fluxes) {
                SubstrateContainer c{VolumeCubicMeters{0.02}, VolumeCubicMeters{iw}, VolumeCubicMeters{0.01}};
                c.reset_balance();
                c.irrigate(VolumeCubicMeters{irr});
                c.process_drainage();
                c.withdraw_transpiration(WaterFluxMolesPerSquareMeterSecond{flux}, AreaSquareMeters{1.0}, Seconds{3600});
                
                if (std::abs(c.get_last_balance().residual_m3) > 1e-10) {
                    numerically_invalid++;
                }
                if (std::isnan(c.current_water().value) || c.current_water().value < 0.0) {
                    numerically_invalid++;
                }
            }
        }
    }
    require(numerically_invalid == 0, "Encountered numerically invalid states during sweep");
}

void test_rootzone_ledger_serialization() {
    using namespace cannaville::rootzone;
    using namespace cannaville::core;
    
    // Use standard smoke scenario to build a valid simulation
    const auto scenario = Scenario::load_json(smoke_json);
    Simulation original(scenario, 99);
    original.advance_fixed_step();
    
    const std::string serialized = original.serialize_state();
    
    require(serialized.find("\"substrate_bulk_volume_m3\"") != std::string::npos, "substrate volume exists");
    require(serialized.find("\"current_water_volume_m3\"") != std::string::npos, "current water volume exists");
    require(serialized.find("\"cumulative_irrigation_top_off_m3\"") != std::string::npos, "irrigation cumulative exists");
    require(serialized.find("\"cumulative_evaporation_m3\"") != std::string::npos, "evaporation cumulative exists");
    
    Simulation restored;
    restored.load_serialized_state(serialized);
    require(restored.serialize_state() == serialized, "roundtrip exact");
}

void test_hydraulic_limitation() {
    using namespace cannaville::rootzone;
    
    // 1. van Genuchten reference
    auto profile_a = get_synthetic_substrate_test_profile_a();
    HydraulicState s;
    evaluate_van_genuchten(0.3, profile_a, s);
    if (s.status != HydraulicStatus::Valid) throw std::runtime_error("vg ref fail");
    
    // 2. theta = theta_s
    evaluate_van_genuchten(0.60, profile_a, s);
    if (s.status != HydraulicStatus::Valid || s.effective_saturation != 1.0) throw std::runtime_error("vg theta_s fail");
    if (s.matric_potential_mpa != 0.0) throw std::runtime_error("vg theta_s mpa fail");

    // 3. theta near theta_r
    evaluate_van_genuchten(0.051, profile_a, s);
    if (s.status != HydraulicStatus::Valid || s.matric_potential_mpa >= -0.01) throw std::runtime_error("vg near theta_r fail");

    // 4. outside supported domain
    evaluate_van_genuchten(0.04, profile_a, s);
    if (s.status != HydraulicStatus::OutsideRetentionModelDomain) throw std::runtime_error("vg outside domain fail");

    // 5. invalid theta_r/theta_s
    auto bad_prof = profile_a;
    bad_prof.theta_r_m3_m3 = 0.6;
    bad_prof.theta_s_m3_m3 = 0.5;
    evaluate_van_genuchten(0.55, bad_prof, s);
    if (s.status != HydraulicStatus::NumericalFailure) throw std::runtime_error("vg bad theta fail");

    // 6. invalid alpha/n
    bad_prof = profile_a;
    bad_prof.n = 0.5;
    evaluate_van_genuchten(0.3, bad_prof, s);
    if (s.status != HydraulicStatus::NumericalFailure) throw std::runtime_error("vg bad n fail");

    // 7. distinct profiles produce different potentials
    auto profile_b = get_synthetic_substrate_test_profile_b();
    HydraulicState sb;
    evaluate_van_genuchten(0.3, profile_b, sb);
    evaluate_van_genuchten(0.3, profile_a, s);
    if (s.matric_potential_mpa == sb.matric_potential_mpa) throw std::runtime_error("vg distinct profiles fail");

    // 8. synthetic psi -> beta
    auto stress_prof = get_synthetic_stress_test_profile();
    evaluate_beta_hydraulic(-1.5, stress_prof, s);
    if (s.status != HydraulicStatus::Valid || s.beta_hydraulic != 0.5) throw std::runtime_error("beta synthetic fail");

    // 9. missing beta profile
    auto miss_stress = get_missing_stress_profile();
    evaluate_beta_hydraulic(-1.5, miss_stress, s);
    if (s.status != HydraulicStatus::MissingHydraulicStressCalibration) throw std::runtime_error("missing beta fail");

    // 10. missing retention
    auto miss_sub = get_missing_substrate_profile();
    evaluate_van_genuchten(0.3, miss_sub, s);
    if (s.status != HydraulicStatus::MissingSubstrateHydraulicProfile) throw std::runtime_error("missing sub fail");

    // 11-13 Medlyn integration
    auto eff = apply_hydraulic_limitation_to_medlyn(3.0, 1.0);
    if (eff.g1_effective != 3.0) throw std::runtime_error("medlyn beta=1 fail");
    eff = apply_hydraulic_limitation_to_medlyn(3.0, 0.5);
    if (eff.g1_effective != 1.5) throw std::runtime_error("medlyn beta=0.5 fail");
    eff = apply_hydraulic_limitation_to_medlyn(3.0, 0.0);
    if (eff.g1_effective != 0.0) throw std::runtime_error("medlyn beta=0 fail");

    // 14-17 DWC policy
    auto dwc = compute_dwc_hydraulic_limitation(true, false, true);
    if (dwc.status != HydraulicStatus::UnrestrictedWaterAccess || dwc.beta_hydraulic.value_or(0.0) != 1.0) throw std::runtime_error("dwc unrestricted fail");
    
    dwc = compute_dwc_hydraulic_limitation(false, false, true);
    if (dwc.status != HydraulicStatus::InsufficientRootzoneWater || dwc.beta_hydraulic.has_value()) throw std::runtime_error("dwc shortage fail");

    dwc = compute_dwc_hydraulic_limitation(true, true, true);
    if (dwc.status != HydraulicStatus::HydraulicStateUnavailable || dwc.beta_hydraulic.has_value()) throw std::runtime_error("dwc geometry fail");

    // Scenario A: two substrates identical VWC
    if (s.matric_potential_mpa == sb.matric_potential_mpa) throw std::runtime_error("scenario A fail");
    
    // Scenario B: Dry down reduces beta
    auto s1 = compute_substrate_hydraulic_limitation(0.5, profile_a, stress_prof);
    auto s2 = compute_substrate_hydraulic_limitation(0.08, profile_a, stress_prof);
    if (s1.beta_hydraulic <= s2.beta_hydraulic) throw std::runtime_error("scenario B fail");

    // Scenario C: mid-step irrigation
    {
        SubstrateContainer sc(cannaville::units::VolumeCubicMeters{1.0}, cannaville::units::VolumeCubicMeters{0.1});
        auto st_before = compute_substrate_hydraulic_limitation(sc.volumetric_water_content(), profile_a, stress_prof);
        sc.irrigate(cannaville::units::VolumeCubicMeters{0.4});
        auto st_after = compute_substrate_hydraulic_limitation(sc.volumetric_water_content(), profile_a, stress_prof);
        if (st_before.beta_hydraulic >= st_after.beta_hydraulic && st_before.beta_hydraulic < 1.0) throw std::runtime_error("Scenario C fail");
    }

    // Scenario D: DWC reservoir loses substantial volume while explicitly maintaining unrestricted water access.
    {
        HydroponicReservoir dwc_res(cannaville::units::VolumeCubicMeters{10.0}, cannaville::units::VolumeCubicMeters{10.0});
        auto dwc_before = compute_dwc_hydraulic_limitation(dwc_res.current_water().value > 0.0, false, true);
        dwc_res.withdraw_transpiration(cannaville::units::WaterFluxMolesPerSquareMeterSecond{100.0}, cannaville::units::AreaSquareMeters{1.0}, cannaville::units::Seconds{3600.0});
        auto dwc_after = compute_dwc_hydraulic_limitation(dwc_res.current_water().value > 0.0, false, true);
        if (dwc_before.beta_hydraulic != 1.0 || dwc_after.beta_hydraulic != 1.0) throw std::runtime_error("Scenario D fail");
    }

    // Sweep for robustness
    for (double vwc = 0.0; vwc <= 1.0; vwc += 0.01) {
        auto st = compute_substrate_hydraulic_limitation(vwc, profile_a, stress_prof);
        if (st.status == HydraulicStatus::NumericalFailure) throw std::runtime_error("sweep numerical failure");
    }
}

void test_end_to_end_substrate() {
    const std::string json = R"json({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1.0",
      "fixed_timestep_seconds": 3600,
      "rooms": [{"id": "room-a", "width_m": 1, "depth_m": 1, "cell_size_m": 1, "cells": [
          {
            "id": "cell-a", "center_x_m": 0.5, "center_y_m": 0.5,
            "environment": {
              "air_temperature_c": 25, "relative_humidity_percent": 50,
              "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450,
              "leaf_temperature_c": 25.0,
              "airflow_m_per_s": 0.5
            },
            "lighting_schedule": [
              {"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000}
            ]
          }
        ]],
      "root_zones": [
        {"id": "rz1", "type": "Substrate", "substrate_bulk_volume_m3": 0.019, "initial_water_volume_m3": 0.009, "substrate_hydraulic_profile_id": "synthetic_test_a", "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"}
      ],
      "plants": [
        {"id": "plant-1", "room_id": "room-a", "cultivar_id": "placeholder", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0, "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.5, "leaf_characteristic_dimension_m": 0.05}
      ]
    })json";
    const auto scenario = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scenario, 42);
    sim.advance_fixed_step();
    
    auto state = sim.full_state_for_internal_use();

    require(state.root_zones.size() == 1, "has root zone");
    require(state.plants.front().latent.realized_water_mol > 0.0, "test_end_to_end_sub failed: transpired water");
    require(state.plants.front().latent.unmet_demand_mol >= 0.0, "valid unmet demand");
}

void test_end_to_end_shared_dwc() {
    const std::string json = R"json({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1.0",
      "fixed_timestep_seconds": 3600,
      "rooms": [{"id": "room-a", "width_m": 2, "depth_m": 1, "cell_size_m": 1, "cells": [
          {
            "id": "cell-a", "center_x_m": 0.5, "center_y_m": 0.5,
            "environment": {
              "air_temperature_c": 25, "relative_humidity_percent": 50,
              "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450,
              "leaf_temperature_c": 25.0,
              "airflow_m_per_s": 0.5
            },
            "lighting_schedule": [
              {"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000}
            ]
          },
          {
            "id": "cell-b", "center_x_m": 1.5, "center_y_m": 0.5,
            "environment": {
              "air_temperature_c": 25, "relative_humidity_percent": 50,
              "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450,
              "leaf_temperature_c": 25.0,
              "airflow_m_per_s": 0.5
            },
            "lighting_schedule": [
              {"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000}
            ]
          }
        ]],
      "root_zones": [
        {"id": "rz1", "type": "Reservoir", "substrate_bulk_volume_m3": 0.1, "initial_water_volume_m3": 0.1, "max_stored_water_m3": 0.1, "explicit_unrestricted_water_access": true}
      ],
      "plants": [
        {"id": "plant-1", "room_id": "room-a", "cultivar_id": "placeholder", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0, "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.5, "leaf_characteristic_dimension_m": 0.05},
        {"id": "plant-2", "room_id": "room-a", "cultivar_id": "placeholder", "root_zone_id": "rz1", "x_m": 1.5, "y_m": 0.5, "z_m": 0, "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.5, "leaf_characteristic_dimension_m": 0.05}
      ]
    })json";
    const auto scenario = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scenario, 42);
    sim.advance_fixed_step();
    
    auto state = sim.full_state_for_internal_use();

    require(state.plants[0].latent.realized_water_mol > 0.0, "test_end_to_end_shared_dwc failed: transpired water");
    require(state.plants[1].latent.realized_water_mol > 0.0, "test_end_to_end_shared_dwc failed: transpired water");
}

void test_end_to_end_two_substrate() {
    const std::string json = R"json({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1.0",
      "fixed_timestep_seconds": 3600,
      "rooms": [{"id": "room-a", "width_m": 2, "depth_m": 1, "cell_size_m": 1, "cells": [
          {
            "id": "cell-a", "center_x_m": 0.5, "center_y_m": 0.5,
            "environment": {
              "air_temperature_c": 25, "relative_humidity_percent": 50,
              "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450,
              "leaf_temperature_c": 25.0,
              "airflow_m_per_s": 0.5
            },
            "lighting_schedule": [
              {"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000}
            ]
          },
          {
            "id": "cell-b", "center_x_m": 1.5, "center_y_m": 0.5,
            "environment": {
              "air_temperature_c": 25, "relative_humidity_percent": 50,
              "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450,
              "leaf_temperature_c": 25.0,
              "airflow_m_per_s": 0.5
            },
            "lighting_schedule": [
              {"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000}
            ]
          }
        ]],
      "root_zones": [
        {"id": "rz1", "type": "Substrate", "substrate_bulk_volume_m3": 0.019, "initial_water_volume_m3": 0.009, "substrate_hydraulic_profile_id": "synthetic_test_a", "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"},
        {"id": "rz2", "type": "Substrate", "substrate_bulk_volume_m3": 0.019, "initial_water_volume_m3": 0.009, "substrate_hydraulic_profile_id": "synthetic_test_a", "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"}
      ],
      "plants": [
        {"id": "plant-1", "room_id": "room-a", "cultivar_id": "placeholder", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0, "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.5, "leaf_characteristic_dimension_m": 0.05},
        {"id": "plant-2", "room_id": "room-a", "cultivar_id": "placeholder", "root_zone_id": "rz2", "x_m": 1.5, "y_m": 0.5, "z_m": 0, "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.5, "leaf_characteristic_dimension_m": 0.05}
      ]
    })json";
    const auto scenario = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scenario, 42);
    sim.advance_fixed_step();
    
    auto state = sim.full_state_for_internal_use();

    require(state.plants[0].latent.realized_water_mol > 0.0, "test_end_to_end_two_substrate failed: transpired water 1");
    require(state.plants[1].latent.realized_water_mol > 0.0, "test_end_to_end_two_substrate failed: transpired water 2");
}

void test_end_to_end_determinism() {
    const std::string json = R"json({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1.0",
      "fixed_timestep_seconds": 3600,
      "rooms": [{"id": "room-a", "width_m": 1, "depth_m": 1, "cell_size_m": 1, "cells": [
          {
            "id": "cell-a", "center_x_m": 0.5, "center_y_m": 0.5,
            "environment": {
              "air_temperature_c": 25, "relative_humidity_percent": 50,
              "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450,
              "leaf_temperature_c": 25.0,
              "airflow_m_per_s": 0.5
            },
            "lighting_schedule": [
              {"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000}
            ]
          }
        ]],
      "root_zones": [
        {"id": "rz1", "type": "Substrate", "substrate_bulk_volume_m3": 0.019, "initial_water_volume_m3": 0.009, "substrate_hydraulic_profile_id": "synthetic_test_a", "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"}
      ],
      "plants": [
        {"id": "plant-1", "room_id": "room-a", "cultivar_id": "placeholder", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0, "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.5, "leaf_characteristic_dimension_m": 0.05}
      ]
    })json";
    const auto scenario = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation first(scenario, 42);
    cannaville::core::Simulation second(scenario, 42);
    first.advance_fixed_step();
    second.advance_fixed_step();
    require(first.serialize_state() == second.serialize_state(), "determinism fail");
}



void test_missing_configurations() {
    const std::string json = R"json({
      "schema_version": 1,
      "scenario_id": "missing_config",
      "simulation_version": "0.1.0",
      "fixed_timestep_seconds": 3600,
      "rooms": [{"id": "r1", "width_m": 1, "depth_m": 1, "cell_size_m": 1, "cells": [
          {
            "id": "c1", "center_x_m": 0.5, "center_y_m": 0.5,
            "environment": {
              "air_temperature_c": 25, "relative_humidity_percent": 50,
              "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450
              
            },
            "lighting_schedule": [
              {"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000}
            ]
          }
        ]],
      "root_zones": [
        {"id": "rz1", "type": "Substrate", "substrate_bulk_volume_m3": 0.019, "initial_water_volume_m3": 0.009, "substrate_hydraulic_profile_id": "synthetic_test_a", "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"}
      ],
      "plants": [
        {"id": "p1", "room_id": "r1", "cultivar_id": "test", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0}
      ]
    })json";
    const auto scenario = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scenario, 42);
    sim.advance_fixed_step();
    
    auto state = sim.full_state_for_internal_use();

    require(state.plants[0].latent.gas_exchange.status == cannaville::gasexchange::ConvergenceStatus::MissingCalibrationProfile, "should be missing profile status");
    require(state.plants[0].latent.transpiration.flux_mol_m2_s == 0.0, "no trans");
}

void test_explicit_synthetic_scenario() {
    const std::string json = R"json({
      "schema_version": 1,
      "scenario_id": "explicit_synth",
      "simulation_version": "0.1.0",
      "fixed_timestep_seconds": 3600,
      "rooms": [{"id": "r1", "width_m": 1, "depth_m": 1, "cell_size_m": 1, "cells": [
          {
            "id": "c1", "center_x_m": 0.5, "center_y_m": 0.5,
            "environment": {
              "air_temperature_c": 25, "relative_humidity_percent": 50,
              "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450,
              "leaf_temperature_c": 25.0,
              "airflow_m_per_s": 0.5
            },
            "lighting_schedule": [
              {"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000}
            ]
          }
        ]],
      "root_zones": [
        {
          "id": "rz1", "type": "Substrate", "substrate_bulk_volume_m3": 0.019, "initial_water_volume_m3": 0.009,
          "substrate_hydraulic_profile_id": "synthetic_test_a",
          "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"
        }
      ],
      "plants": [
        {
          "id": "p1", "room_id": "r1", "cultivar_id": "test", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0,
          "gas_exchange_profile_id": "synthetic_vegetative_test",
          "effective_transpiring_leaf_area_m2": 0.5,
          "leaf_characteristic_dimension_m": 0.05
        }
      ]
    })json";
    const auto scenario = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scenario, 42);
    sim.advance_fixed_step();
    
    auto state = sim.full_state_for_internal_use();

    require(state.plants[0].latent.gas_exchange.status == cannaville::gasexchange::ConvergenceStatus::Converged, "should converge with explicit synth");
    require(state.plants[0].latent.transpiration.flux_mol_m2_s > 0.0, "should transpire");
}

void test_mid_step_irrigation_0807() {
    const std::string json = R"json({
      "schema_version": 1,
      "scenario_id": "mid_step_irrig",
      "simulation_version": "0.1.0",
      "fixed_timestep_seconds": 3600,
      "rooms": [{"id": "r1", "width_m": 1, "depth_m": 1, "cell_size_m": 1, "cells": [
          {
            "id": "c1", "center_x_m": 0.5, "center_y_m": 0.5,
            "environment": {
              "air_temperature_c": 25, "relative_humidity_percent": 50,
              "atmospheric_pressure_kpa": 101.325, "co2_umol_per_mol": 450,
              "leaf_temperature_c": 25.0,
              "airflow_m_per_s": 0.5
            },
            "lighting_schedule": [
              {"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000}
            ]
          }
        ]],
      "root_zones": [
        {
          "id": "rz1", "type": "Substrate", "substrate_bulk_volume_m3": 0.019, "initial_water_volume_m3": 0.009,
          "substrate_hydraulic_profile_id": "synthetic_test_a",
          "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"
        }
      ],
      "plants": [
        {
          "id": "p1", "room_id": "r1", "cultivar_id": "test", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0,
          "gas_exchange_profile_id": "synthetic_vegetative_test",
          "effective_transpiring_leaf_area_m2": 0.5,
          "leaf_characteristic_dimension_m": 0.05
        }
      ],
      "water_events": [
        {
          "id": "we1", "timestamp_s": 1800, "root_zone_id": "rz1", "type": "irrigation", "amount_m3": 0.005
        }
      ]
    })json";
    const auto scenario = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scenario, 42);
    sim.advance_fixed_step();
    
    auto state = sim.full_state_for_internal_use();

    require(state.root_zones[0].cumulative_irrigation_top_off.value == 0.005, "should apply irrig mid-step");
}



void test_csv_schema_and_values() {
    cannaville::core::Simulation sim(cannaville::core::Scenario::load_json(smoke_json), 42);
    sim.advance_fixed_step();
    std::string header = sim.root_zone_csv_header();
    std::string row = sim.root_zone_csv_row();
    
    require(header.find("interval_start_storage_m3") != std::string::npos, "CSV must contain interval_start_storage_m3");
    require(header.find("interval_end_storage_m3") != std::string::npos, "CSV must contain interval_end_storage_m3");
    require(header.find("capacity_m3") != std::string::npos, "CSV must contain capacity_m3");
    require(header.find("cumulative_irrigation_m3") != std::string::npos, "CSV must contain cumulative_irrigation_m3");
    require(header.find("cumulative_drainage_m3") != std::string::npos, "CSV must contain cumulative_drainage_m3");
    require(header.find("cumulative_withdrawal_m3") != std::string::npos, "CSV must contain cumulative_withdrawal_m3");
    require(header.find("conservation_residual_m3") != std::string::npos, "CSV must contain conservation_residual_m3");
    
    require(row.find("0.000000") != std::string::npos, "Conservation residual should be 0");
}

void test_save_load_event_and_conservation_continuation() {
    std::string json = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "r", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "root_zones": [
        {
          "id": "rz1", "type": "Substrate", "substrate_bulk_volume_m3": 0.01, 
          "initial_water_volume_m3": 0.00101, "max_stored_water_m3": 0.01,
          "substrate_hydraulic_profile_id": "synthetic_test_a",
          "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"
        }
      ],
      "plants": [
        {
          "id": "p1", "room_id": "r", "cultivar_id": "c", "root_zone_id": "rz1", 
          "x_m": 0, "y_m": 0, "z_m": 0,
          "gas_exchange_profile_id": "synthetic_vegetative_test",
          "effective_transpiring_leaf_area_m2": 0.1,
          "leaf_characteristic_dimension_m": 0.05
        }
      ],
      "water_events": [
        {"id": "e1", "timestamp_s": 1800, "root_zone_id": "rz1", "type": "irrigation", "amount_m3": 0.001}
      ]
    })";
    
    // Add missing environment
    auto parsed = cannaville::core::Scenario::load_json(json);
    cannaville::core::ScenarioCellDefinition cell_def;
        cell_def.id = "c1";
        cell_def.center_x.value = 0.5;
        cell_def.center_y.value = 0.5;
        cell_def.environment.air_temperature.value = 25.0;
        cell_def.environment.relative_humidity.value = 50.0;
        cell_def.environment.atmospheric_pressure.value = 101.3;
        cell_def.environment.carbon_dioxide.value = 400.0;
        cell_def.environment.airflow = cannaville::units::AirflowMetersPerSecond{1.0};
        cell_def.environment.leaf_temperature = cannaville::units::Celsius{25.0};
        cell_def.environment.leaf_vpd = cannaville::units::VPDKPa{1.5};
        cell_def.environment.physics_available = true;
        parsed.rooms[0].cells.push_back(cell_def);
    parsed.rooms[0].cells[0].lighting_schedule.segments.push_back({0, 86400, 1000.0});
    
    // Execution A
    cannaville::core::Simulation simA(parsed, 42);
    simA.advance_fixed_step();
    simA.advance_fixed_step();
    simA.advance_fixed_step();
    std::string csv_A = simA.root_zone_csv_row();
    std::string csv_p_A = simA.plant_physiology_csv_row();
    
    // Execution B
    cannaville::core::Simulation simB(parsed, 42);
    simB.advance_fixed_step(); // t=900
    std::string state_str = simB.serialize_state();
    simB.load_serialized_state(state_str);
    simB.advance_fixed_step(); // t=1800 (event happens)
    simB.advance_fixed_step(); // t=2700
    std::string csv_B = simB.root_zone_csv_row();
    std::string csv_p_B = simB.plant_physiology_csv_row();
    
    if (csv_A != csv_B || csv_p_A != csv_p_B) { std::cout << "pA:\n" << csv_p_A << "pB:\n" << csv_p_B << std::endl; }
    require(csv_A == csv_B, "Root zone output must match after save/load");
    require(csv_p_A == csv_p_B, "Physiology output must match after save/load");
}

void test_two_substrate_causal_chain() {
    std::string json = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "r", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "root_zones": [
          {"id": "rz_a", "type": "Substrate", "substrate_bulk_volume_m3": 0.05, "initial_water_volume_m3": 0.00508, "max_stored_water_m3": 0.05, "substrate_hydraulic_profile_id": "synthetic_test_a", "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"},
          {"id": "rz_b", "type": "Substrate", "substrate_bulk_volume_m3": 0.05, "initial_water_volume_m3": 0.00508, "max_stored_water_m3": 0.05, "substrate_hydraulic_profile_id": "synthetic_test_b", "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"}
      ],
      "plants": [
          {"id": "p_a", "room_id": "r", "cultivar_id": "c", "root_zone_id": "rz_a", "x_m": 0.5, "y_m": 0.5, "z_m": 0, "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.1, "leaf_characteristic_dimension_m": 0.05},
          {"id": "p_b", "room_id": "r", "cultivar_id": "c", "root_zone_id": "rz_b", "x_m": 1.5, "y_m": 0.5, "z_m": 0, "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.1, "leaf_characteristic_dimension_m": 0.05}
      ],
      "water_events": []
    })";
    auto parsed = cannaville::core::Scenario::load_json(json);
    cannaville::core::ScenarioCellDefinition cell_def;
    cell_def.id = "c1"; cell_def.center_x.value = 0.5; cell_def.center_y.value = 0.5;
    cell_def.environment.air_temperature.value = 25.0; cell_def.environment.relative_humidity.value = 50.0;
    cell_def.environment.atmospheric_pressure.value = 101.3; cell_def.environment.carbon_dioxide.value = 400.0;
    cell_def.environment.airflow = cannaville::units::AirflowMetersPerSecond{1.0};
    cell_def.environment.leaf_temperature = cannaville::units::Celsius{25.0};
    cell_def.environment.leaf_vpd = cannaville::units::VPDKPa{1.5};
    cell_def.environment.physics_available = true;
    cell_def.lighting_schedule.segments.push_back({0, 86400, 1000.0});
    parsed.rooms[0].cells.push_back(cell_def);
    
    cannaville::core::ScenarioCellDefinition cell_def_b = cell_def;
    cell_def_b.id = "c2"; cell_def_b.center_x.value = 1.5;
    parsed.rooms[0].cells.push_back(cell_def_b);
    
    cannaville::core::Simulation sim(parsed, 42);
    
    auto st_initial = sim.full_state_for_internal_use();
    double init_vwc_a = st_initial.root_zones[0].volumetric_water_content.value();
    double init_vwc_b = st_initial.root_zones[1].volumetric_water_content.value();
    require_near(init_vwc_a, init_vwc_b, 1e-9, "Initial VWC should be equal");
    
    sim.advance_fixed_step();
    auto st = sim.full_state_for_internal_use();
    
    double psi_a = st.root_zones[0].matric_potential.value();
    double psi_b = st.root_zones[1].matric_potential.value();
    require(std::abs(psi_a - psi_b) > 0.01, "Matric potential should differ");
    
    double beta_a = st.plants[0].latent.gas_exchange.beta_hydraulic.value_or(1.0);
    double beta_b = st.plants[1].latent.gas_exchange.beta_hydraulic.value_or(1.0);
    require(std::abs(beta_a - beta_b) > 0.01, "Beta should differ");
    
    double g1_a = st.plants[0].latent.gas_exchange.g1_effective;
    double g1_b = st.plants[1].latent.gas_exchange.g1_effective;
    require(std::abs(g1_a - g1_b) > 0.01, "g1_effective should differ");
    
    double gs_a = st.plants[0].latent.gas_exchange.stomatal_conductance.value;
    double gs_b = st.plants[1].latent.gas_exchange.stomatal_conductance.value;
    require(std::abs(gs_a - gs_b) > 0.001, "Stomatal conductance should differ");
    
    double tr_a = st.plants[0].latent.requested_water_mol;
    double tr_b = st.plants[1].latent.requested_water_mol;
    require(std::abs(tr_a - tr_b) > 0.01, "Requested water should differ");
}

void test_shared_dwc_shortage_and_reorder() {
    std::string jsonA = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "r", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "root_zones": [
        {
          "id": "rz1", "type": "Reservoir",
          "initial_water_volume_m3": 0.0000001, "max_stored_water_m3": 0.01,
          "explicit_unrestricted_water_access": true
        }
      ],
      "plants": [
        {
          "id": "pA", "room_id": "r", "cultivar_id": "c", "root_zone_id": "rz1", 
          "x_m": 0, "y_m": 0, "z_m": 0,
          "gas_exchange_profile_id": "synthetic_vegetative_test",
          "effective_transpiring_leaf_area_m2": 0.1,
          "leaf_characteristic_dimension_m": 0.05
        },
        {
          "id": "pB", "room_id": "r", "cultivar_id": "c", "root_zone_id": "rz1", 
          "x_m": 1, "y_m": 0, "z_m": 0,
          "gas_exchange_profile_id": "synthetic_vegetative_test",
          "effective_transpiring_leaf_area_m2": 0.2,
          "leaf_characteristic_dimension_m": 0.05
        }
      ]
    })";
    
    auto parsedA = cannaville::core::Scenario::load_json(jsonA);
    cannaville::core::ScenarioCellDefinition cell_def;
        cell_def.id = "c1";
        cell_def.center_x.value = 0.5;
        cell_def.center_y.value = 0.5;
        cell_def.environment.air_temperature.value = 25.0;
        cell_def.environment.relative_humidity.value = 50.0;
        cell_def.environment.atmospheric_pressure.value = 101.3;
        cell_def.environment.carbon_dioxide.value = 400.0;
        cell_def.environment.airflow = cannaville::units::AirflowMetersPerSecond{1.0};
        cell_def.environment.leaf_temperature = cannaville::units::Celsius{25.0};
        cell_def.environment.leaf_vpd = cannaville::units::VPDKPa{1.5};
        cell_def.environment.physics_available = true;
        parsedA.rooms[0].cells.push_back(cell_def);
    parsedA.rooms[0].cells[0].lighting_schedule.segments.push_back({0, 86400, 1000.0});
    
    auto parsedB = parsedA; // Reverse plant order
    std::swap(parsedB.plants[0], parsedB.plants[1]);
    
    cannaville::core::Simulation simA(parsedA, 42);
    cannaville::core::Simulation simB(parsedB, 42);
    
    simA.advance_fixed_step();
    simB.advance_fixed_step();
    
    double ud_A_A = simA.full_state_for_internal_use().plants[0].latent.unmet_demand_mol;
    double ud_A_B = simA.full_state_for_internal_use().plants[1].latent.unmet_demand_mol;
    
    double ud_B_A = simB.full_state_for_internal_use().plants[1].latent.unmet_demand_mol;
    double ud_B_B = simB.full_state_for_internal_use().plants[0].latent.unmet_demand_mol;
    
    require(ud_A_A > 0 && ud_A_B > 0, "Must have unmet demand");
    require_near(ud_A_A, ud_B_A, 1e-9, "Order must not affect proportional allocation A");
    require_near(ud_A_B, ud_B_B, 1e-9, "Order must not affect proportional allocation B");
    require_near(ud_A_B, ud_A_A * 2.0, 1e-4, "Perfectly proportional allocation");
    
    require(simA.full_state_for_internal_use().root_zones[0].current_water_volume.value >= 0.0, "No negative reservoir");
}

void test_mid_step_irrigation_segmented() {
    std::string json = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "r", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "root_zones": [
        {
          "id": "rz1", "type": "Substrate", "substrate_bulk_volume_m3": 0.01, 
          "initial_water_volume_m3": 0.00101, "max_stored_water_m3": 0.01,
          "substrate_hydraulic_profile_id": "synthetic_test_a",
          "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"
        }
      ],
      "plants": [
        {
          "id": "p1", "room_id": "r", "cultivar_id": "c", "root_zone_id": "rz1", 
          "x_m": 0, "y_m": 0, "z_m": 0,
          "gas_exchange_profile_id": "synthetic_vegetative_test",
          "effective_transpiring_leaf_area_m2": 0.1,
          "leaf_characteristic_dimension_m": 0.05
        }
      ],
      "water_events": [
        {"id": "e1", "timestamp_s": 420, "root_zone_id": "rz1", "type": "irrigation", "amount_m3": 0.004}
      ]
    })";
    
    auto parsed = cannaville::core::Scenario::load_json(json);
    cannaville::core::ScenarioCellDefinition cell_def;
        cell_def.id = "c1";
        cell_def.center_x.value = 0.5;
        cell_def.center_y.value = 0.5;
        cell_def.environment.air_temperature.value = 25.0;
        cell_def.environment.relative_humidity.value = 50.0;
        cell_def.environment.atmospheric_pressure.value = 101.3;
        cell_def.environment.carbon_dioxide.value = 400.0;
        cell_def.environment.airflow = cannaville::units::AirflowMetersPerSecond{1.0};
        cell_def.environment.leaf_temperature = cannaville::units::Celsius{25.0};
        cell_def.environment.leaf_vpd = cannaville::units::VPDKPa{1.5};
        cell_def.environment.physics_available = true;
        parsed.rooms[0].cells.push_back(cell_def);
    parsed.rooms[0].cells[0].lighting_schedule.segments.push_back({0, 86400, 1000.0});
    
    cannaville::core::Simulation sim(parsed, 42);
    sim.advance_fixed_step(); // 900s step, irrigation at 420s (7 minutes in).
    
    // Check that we got irrigation
    require(sim.full_state_for_internal_use().root_zones[0].cumulative_irrigation_top_off.value == 0.004, "Must irrigate 0.004");
    require(sim.full_state_for_internal_use().plants[0].latent.gas_exchange.beta_hydraulic.value_or(0.0) == 1.0, "Instantaneous rate reflects final wet segment");
}

void test_mid_step_lighting_segmented() {
    std::string json = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "r", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "root_zones": [
        {
          "id": "rz1", "type": "Reservoir",
          "initial_water_volume_m3": 0.01, "max_stored_water_m3": 0.01,
          "explicit_unrestricted_water_access": true
        }
      ],
      "plants": [
        {
          "id": "p1", "room_id": "r", "cultivar_id": "c", "root_zone_id": "rz1", 
          "x_m": 0, "y_m": 0, "z_m": 0,
          "gas_exchange_profile_id": "synthetic_vegetative_test",
          "effective_transpiring_leaf_area_m2": 0.1,
          "leaf_characteristic_dimension_m": 0.05
        }
      ]
    })";
    
    auto parsed = cannaville::core::Scenario::load_json(json);
    cannaville::core::ScenarioCellDefinition cell_def;
        cell_def.id = "c1";
        cell_def.center_x.value = 0.5;
        cell_def.center_y.value = 0.5;
        cell_def.environment.air_temperature.value = 25.0;
        cell_def.environment.relative_humidity.value = 50.0;
        cell_def.environment.atmospheric_pressure.value = 101.3;
        cell_def.environment.carbon_dioxide.value = 400.0;
        cell_def.environment.airflow = cannaville::units::AirflowMetersPerSecond{1.0};
        cell_def.environment.leaf_temperature = cannaville::units::Celsius{25.0};
        cell_def.environment.leaf_vpd = cannaville::units::VPDKPa{1.5};
        cell_def.environment.physics_available = true;
        parsed.rooms[0].cells.push_back(cell_def);
    // Light changes at 450s mid-step
    parsed.rooms[0].cells[0].lighting_schedule.segments.push_back({0, 450, 0.0});
    parsed.rooms[0].cells[0].lighting_schedule.segments.push_back({450, 86400, 1000.0});
    
    cannaville::core::Simulation sim(parsed, 42);
    sim.advance_fixed_step();
    // Verify some assimilation happened (so lights came on), but not as much as full 900s
    require(sim.full_state_for_internal_use().plants[0].latent.gas_exchange.net_assimilation.value > 0.0, "Must have assimilated something");
    require(sim.full_state_for_internal_use().plants[0].latent.gas_exchange.net_assimilation.value > 10.0, "Instantaneous rate reflects final lit segment");
}

void test_missing_configurations_split() {
    std::string base_json = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "r", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "root_zones": [
        {
          "id": "rz1", "type": "Substrate", "substrate_bulk_volume_m3": 0.01, 
          "initial_water_volume_m3": 0.00101, "max_stored_water_m3": 0.01,
          "substrate_hydraulic_profile_id": "synthetic_test_a",
          "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"
        }
      ],
      "plants": [
        {
          "id": "p1", "room_id": "r", "cultivar_id": "c", "root_zone_id": "rz1", 
          "x_m": 0, "y_m": 0, "z_m": 0,
          "gas_exchange_profile_id": "synthetic_vegetative_test",
          "effective_transpiring_leaf_area_m2": 0.1,
          "leaf_characteristic_dimension_m": 0.05
        }
      ]
    })";

    auto get_sim = [&](auto modify) {
        auto parsed = cannaville::core::Scenario::load_json(base_json);
        cannaville::core::ScenarioCellDefinition cell_def;
        cell_def.id = "c1";
        cell_def.center_x.value = 0.5;
        cell_def.center_y.value = 0.5;
        cell_def.environment.air_temperature.value = 25.0;
        cell_def.environment.relative_humidity.value = 50.0;
        cell_def.environment.atmospheric_pressure.value = 101.3;
        cell_def.environment.carbon_dioxide.value = 400.0;
        cell_def.environment.airflow = cannaville::units::AirflowMetersPerSecond{1.0};
        cell_def.environment.leaf_temperature = cannaville::units::Celsius{25.0};
        cell_def.environment.leaf_vpd = cannaville::units::VPDKPa{1.5};
        cell_def.environment.physics_available = true;
        parsed.rooms[0].cells.push_back(cell_def);
        parsed.rooms[0].cells[0].lighting_schedule.segments.push_back({0, 86400, 1000.0});
        modify(parsed);
        cannaville::core::Simulation sim(parsed, 42);
        sim.advance_fixed_step();
        return sim;
    };

    // Missing Gas Profile
    auto s1 = get_sim([](auto& p) { p.plants[0].gas_exchange_profile_id = std::nullopt; });
    require(s1.full_state_for_internal_use().plants[0].latent.gas_exchange.status == cannaville::gasexchange::ConvergenceStatus::MissingGasExchangeProfile, "MissingGasExchangeProfile");

    // Missing Substrate Hydraulic Profile
    auto s2 = get_sim([](auto& p) { p.root_zones[0].substrate_hydraulic_profile_id = std::nullopt; });
    require(s2.full_state_for_internal_use().plants[0].latent.gas_exchange.status == cannaville::gasexchange::ConvergenceStatus::MissingSubstrateHydraulicProfile, "MissingSubstrateHydraulicProfile");

    // Missing Leaf Temp
    auto s3 = get_sim([](auto& p) { p.rooms[0].cells[0].environment.leaf_temperature = std::nullopt; });
    require(s3.full_state_for_internal_use().plants[0].latent.gas_exchange.status == cannaville::gasexchange::ConvergenceStatus::MissingLeafTemperature, "MissingLeafTemperature");
}




void test_fresh_simulation_conservation_residual() {
    std::string json = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 3600,
      "rooms": [{"id": "r", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "root_zones": [
          {
              "id": "rz1",
              "type": "Substrate",
              "substrate_bulk_volume_m3": 0.05,
              "initial_water_volume_m3": 0.015,
              "substrate_hydraulic_profile_id": "synthetic_test_a",
              "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"
          }
      ],
      "plants": [
          {"id": "p", "room_id": "r", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0.0, "cultivar_id": "c1", "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.1, "leaf_characteristic_dimension_m": 0.05}
      ],
      "water_events": []
    })";
    cannaville::core::Scenario scen = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scen, 42);
    auto st = sim.full_state_for_internal_use();
    require(st.root_zones[0].interval_start_water_volume.value == 0.015, "Initial water must be populated");
    double residual = st.root_zones[0].current_water_volume.value - (st.root_zones[0].interval_start_water_volume.value + st.root_zones[0].cumulative_irrigation_top_off.value + st.root_zones[0].cumulative_external_return_flow.value - st.root_zones[0].cumulative_drainage_discharge.value - st.root_zones[0].cumulative_realized_withdrawal.value - st.root_zones[0].cumulative_evaporation.value);
    require(std::abs(residual) < 1e-9, "Residual must be 0 before step");
    sim.advance_fixed_step();
    st = sim.full_state_for_internal_use();
    residual = st.root_zones[0].current_water_volume.value - (st.root_zones[0].interval_start_water_volume.value + st.root_zones[0].cumulative_irrigation_top_off.value + st.root_zones[0].cumulative_external_return_flow.value - st.root_zones[0].cumulative_drainage_discharge.value - st.root_zones[0].cumulative_realized_withdrawal.value - st.root_zones[0].cumulative_evaporation.value);
    require(std::abs(residual) < 1e-9, "Residual must be 0 after step");
}

void test_substrate_overflow_capacity() {
    std::string json = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "r", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "root_zones": [
          {
              "id": "rz1",
              "type": "Substrate",
              "substrate_bulk_volume_m3": 0.05,
              "max_stored_water_m3": 0.02,
              "initial_water_volume_m3": 0.015,
              "substrate_hydraulic_profile_id": "synthetic_test_a",
              "hydraulic_stress_transfer_profile_id": "synthetic_stress_test"
          }
      ],
      "plants": [
          {"id": "p", "room_id": "r", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0.0, "cultivar_id": "c1", "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.1, "leaf_characteristic_dimension_m": 0.05}
      ],
      "water_events": [
          {"id": "w1", "timestamp_s": 0, "root_zone_id": "rz1", "type": "irrigation", "amount_m3": 0.01}
      ]
    })";
    cannaville::core::Scenario scen = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scen, 42);
    sim.advance_fixed_step();
    auto st = sim.full_state_for_internal_use();
    require(st.root_zones[0].current_water_volume.value <= 0.02, "Storage must not exceed capacity");
    require(st.root_zones[0].cumulative_drainage_discharge.value > 0.0, "Drainage must occur explicitly");
    double residual = st.root_zones[0].current_water_volume.value - (st.root_zones[0].interval_start_water_volume.value + st.root_zones[0].cumulative_irrigation_top_off.value + st.root_zones[0].cumulative_external_return_flow.value - st.root_zones[0].cumulative_drainage_discharge.value - st.root_zones[0].cumulative_realized_withdrawal.value - st.root_zones[0].cumulative_evaporation.value);
    require(std::abs(residual) < 1e-9, "Conservation closes");
}

void test_reservoir_overflow_capacity() {
    std::string json = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "r", "width_m": 4, "depth_m": 4, "cell_size_m": 1}],
      "root_zones": [
          {
              "id": "rz1",
              "type": "Reservoir",
              "max_stored_water_m3": 100.0,
              "initial_water_volume_m3": 95.0,
              "explicit_unrestricted_water_access": true
          }
      ],
      "plants": [
          {"id": "p", "room_id": "r", "root_zone_id": "rz1", "x_m": 0.5, "y_m": 0.5, "z_m": 0.0, "cultivar_id": "c1", "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.1, "leaf_characteristic_dimension_m": 0.05}
      ],
      "water_events": [
          {"id": "w1", "timestamp_s": 0, "root_zone_id": "rz1", "type": "top_off", "amount_m3": 10.0}
      ]
    })";
    cannaville::core::Scenario scen = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scen, 42);
    sim.advance_fixed_step();
    auto st = sim.full_state_for_internal_use();
    require(st.root_zones[0].current_water_volume.value <= 100.0, "Storage must not exceed capacity");
    require(st.root_zones[0].cumulative_drainage_discharge.value >= 4.9, "Discharge must occur explicitly");
}

void test_multicell_physiology_csv_spatial() {
    std::string json = R"({
      "schema_version": 1,
      "scenario_id": "test",
      "simulation_version": "0.1",
      "fixed_timestep_seconds": 900,
      "rooms": [{"id": "r", "width_m": 2, "depth_m": 1, "cell_size_m": 1,
          "cells": [
              {"id": "cell-0-0", "center_x_m": 0.5, "center_y_m": 0.5, "environment": {"air_temperature_c": 25.0, "relative_humidity_percent": 50.0, "co2_umol_per_mol": 400.0, "atmospheric_pressure_kpa": 101.325, "airflow_m_per_s": 1.0}, "lighting_schedule": [{"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 500.0}]},
              {"id": "cell-1-0", "center_x_m": 1.5, "center_y_m": 0.5, "environment": {"air_temperature_c": 25.0, "relative_humidity_percent": 50.0, "co2_umol_per_mol": 400.0, "atmospheric_pressure_kpa": 101.325, "airflow_m_per_s": 2.0}, "lighting_schedule": [{"start_seconds": 0, "end_seconds": 86400, "ppfd_umol_per_m2_s": 1000.0}]}
          ]
      }],
      "root_zones": [{"id": "rz1", "type": "Reservoir", "max_stored_water_m3": 100.0, "initial_water_volume_m3": 95.0, "explicit_unrestricted_water_access": true}],
      "plants": [
          {"id": "p1", "room_id": "r", "root_zone_id": "rz1", "x_m": 1.5, "y_m": 0.5, "z_m": 0.0, "cultivar_id": "c1", "gas_exchange_profile_id": "synthetic_vegetative_test", "effective_transpiring_leaf_area_m2": 0.1, "leaf_characteristic_dimension_m": 0.05}
      ],
      "water_events": []
    })";
    cannaville::core::Scenario scen = cannaville::core::Scenario::load_json(json);
    cannaville::core::Simulation sim(scen, 42);
    sim.advance_fixed_step();
    std::string csv = sim.plant_physiology_csv_row();
    std::cout << "CSV:" << csv << "\n";
    require(csv.find("cell-1-0") != std::string::npos, "Must report the correct cell");
    require(csv.find("1000.000000") != std::string::npos, "Must report PPFD from cell-1-0");
}

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
        test_rootzone_dry_down();
        test_rootzone_pulsed_irrigation();
        test_rootzone_dwc_reservoir();
        test_rootzone_shared_allocation();
        test_rootzone_exact_event_timing();
        test_rootzone_unit_conversion();
        test_rootzone_robustness_sweep();
        test_rootzone_ledger_serialization();
        test_hydraulic_limitation();

        test_fresh_simulation_conservation_residual();
        std::cout << "PASS: Fresh simulation residual result" << std::endl;
        test_substrate_overflow_capacity();
        std::cout << "PASS: Substrate overflow event result" << std::endl;
        test_reservoir_overflow_capacity();
        std::cout << "PASS: Reservoir overflow event result" << std::endl;
        test_multicell_physiology_csv_spatial();
        std::cout << "PASS: Multi-cell physiology CSV spatial result" << std::endl;

        
        // P1B.4.1C explicit requested tests
        test_csv_schema_and_values();
        std::cout << "PASS: CSV schema and value reconstruction" << std::endl;
        
        test_save_load_event_and_conservation_continuation();
        std::cout << "PASS: Save/load continuation and event persistence" << std::endl;
        
        test_two_substrate_causal_chain();
        std::cout << "PASS: Two-substrate causal chain" << std::endl;
        
        test_shared_dwc_shortage_and_reorder();
        std::cout << "PASS: Shared DWC proportional allocation and reverse order equivalence" << std::endl;
        
        test_mid_step_irrigation_segmented();
        std::cout << "PASS: Mid-step irrigation physiological segmentation" << std::endl;
        
        test_mid_step_lighting_segmented();
        std::cout << "PASS: Mid-step lighting transition physiological segmentation" << std::endl;
        
        test_missing_configurations_split();
        std::cout << "PASS: Individual missing configuration specific status validation" << std::endl;
        
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
