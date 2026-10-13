#include "Simulation/Consequences/ConsequenceState.hpp"
#include "Simulation/Core/Scenario.hpp"
#include "Simulation/Core/Simulation.hpp"
#include "Simulation/Environment/VaporPressure.hpp"
#include "Simulation/LeafEnergy/LeafEnergy.hpp"
#include "Simulation/GasExchange/GasExchange.hpp"
#include "Simulation/RootZone/HydraulicLimitation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

using cannaville::consequences::DevelopmentStage;
using cannaville::consequences::PlantConsequencePresentation;
using cannaville::consequences::PlantConsequenceProfile;
using cannaville::consequences::PlantConsequenceState;
using cannaville::consequences::SegmentInputs;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void require_near(double actual, double expected, double tolerance, const char* message) {
    if (std::abs(actual - expected) > tolerance) throw std::runtime_error(message);
}

PlantConsequenceProfile profile() {
    auto value = cannaville::consequences::get_synthetic_test_profile();
    value.stages[0].required_opportunity_s = 60.0;
    value.stages[1].required_opportunity_s = 120.0;
    value.stages[2].required_opportunity_s = 180.0;
    value.stages[3].required_opportunity_s = 1.0;
    for (auto& stage : value.stages) stage.assimilation_reference_umol_m2_s = 10.0;
    return value;
}

SegmentInputs good(double start, double duration) {
    SegmentInputs input;
    input.start_time_s = start;
    input.duration_s = duration;
    input.net_assimilation_umol_m2_s = 10.0;
    input.hydraulic_beta = 1.0;
    input.water_allocation_available = true;
    input.requested_water_mol = 1.0;
    input.realized_water_mol = 1.0;
    input.leaf_temperature_c = 24.0;
    input.absorbed_shortwave_w_m2 = 100.0;
    input.gas_exchange_status = "Converged";
    input.transpiration_status = "Valid";
    return input;
}

void test_opportunity_arithmetic_and_monotonic_loss() {
    PlantConsequenceState state;
    PlantConsequencePresentation presentation;
    auto input = good(0.0, 10.0);
    cannaville::consequences::integrate_segment(state, presentation, profile(), input);
    require_near(state.potential_opportunity_s, 10.0, 1e-12, "potential opportunity");
    require_near(state.achieved_opportunity_s, 10.0, 1e-12, "achieved opportunity");
    require_near(state.lost_opportunity_s, 0.0, 1e-12, "lost opportunity");
    input.net_assimilation_umol_m2_s = 5.0;
    input.start_time_s = 10.0;
    cannaville::consequences::integrate_segment(state, presentation, profile(), input);
    require_near(state.lost_opportunity_s, 5.0, 1e-12, "lost opportunity increment");
    require_near(state.potential_opportunity_s, state.achieved_opportunity_s + state.lost_opportunity_s, 1e-12, "opportunity identity");
}

void test_lifecycle_and_exact_transition_remainder() {
    PlantConsequenceState state;
    PlantConsequencePresentation presentation;
    auto input = good(0.0, 90.0);
    cannaville::consequences::integrate_segment(state, presentation, profile(), input);
    require(state.stage == DevelopmentStage::Vegetative, "stage did not transition within segment");
    require_near(state.stage_opportunity_accumulated_s, 30.0, 1e-12, "next-stage remainder was discarded");
    require_near(state.stage_elapsed_s, 30.0, 1e-12, "stage elapsed remainder");
    require_near(state.stage_progress_0_1, 0.25, 1e-12, "next-stage progress");
}

void test_explicit_stage_gates() {
    auto p = profile();
    p.stages[0].minimum_elapsed_stage_s = 100.0;
    PlantConsequenceState state;
    PlantConsequencePresentation presentation;
    cannaville::consequences::integrate_segment(state, presentation, p, good(0.0, 60.0));
    require(state.stage == DevelopmentStage::Seedling, "minimum stage-time gate was ignored");
    require_near(state.stage_progress_0_1, 1.0, 1e-12, "completed gated stage progress");
    auto second = good(60.0, 40.0);
    cannaville::consequences::integrate_segment(state, presentation, p, second);
    require(state.stage == DevelopmentStage::Vegetative, "minimum stage-time gate did not open");

    p = profile();
    p.stages[0].minimum_dark_duration_s = 100.0;
    state = {};
    cannaville::consequences::integrate_segment(state, presentation, p, good(0.0, 60.0));
    require(state.stage == DevelopmentStage::Seedling, "dark-duration gate was ignored");
    auto dark = good(60.0, 1.0);
    dark.accumulated_dark_duration_s = 100.0;
    cannaville::consequences::integrate_segment(state, presentation, p, dark);
    require(state.stage == DevelopmentStage::Vegetative, "dark-duration gate did not open");
}

void test_water_thermal_radiation_and_recovery() {
    PlantConsequenceState state;
    PlantConsequencePresentation presentation;
    auto p = profile();
    p.water.onset = 0.2; p.water.full = 0.8; p.water.damage_onset = 0.4;
    p.thermal.onset = 25.0; p.thermal.full = 35.0; p.thermal.damage_onset = 0.5;
    p.radiation.onset = 100.0; p.radiation.full = 300.0; p.radiation.damage_onset = 0.5;
    auto input = good(0.0, 10.0);
    input.hydraulic_beta = 0.0;
    input.realized_water_mol = 0.0;
    input.leaf_temperature_c = 35.0;
    input.absorbed_shortwave_w_m2 = 300.0;
    cannaville::consequences::integrate_segment(state, presentation, p, input);
    require(state.water_current_severity_0_1 > 0.0, "water severity missing");
    require(state.thermal_current_severity_0_1 > 0.0, "thermal severity missing");
    require(state.radiation_current_severity_0_1 > 0.0, "radiation severity missing");
    require(state.water_damage_0_1 > 0.0, "water damage missing");
    require(presentation.wilt_severity_0_1 > 0.0, "wilt symptom missing");
    bool water_reason = false;
    for (const auto& reason : presentation.reason_codes) {
        if (reason.reason_code == "WATER_SUPPLY_LIMITED") {
            water_reason = true;
            require(!reason.supporting_variables.empty(), "water reason lacks supporting evidence");
        }
    }
    require(water_reason, "water reason code missing");
    input.start_time_s = 10.0;
    input.hydraulic_beta = 1.0;
    input.realized_water_mol = 1.0;
    input.leaf_temperature_c = 24.0;
    input.absorbed_shortwave_w_m2 = 100.0;
    cannaville::consequences::integrate_segment(state, presentation, p, input);
    require(state.water_recovery_0_1 < 1.0, "water recovery did not decline");
    require(state.water_damage_0_1 > 0.0, "persistent water damage was erased");
}

void test_history_dependence_and_determinism() {
    const auto p = profile();
    PlantConsequenceState stressed;
    PlantConsequencePresentation stressed_view;
    auto bad = good(0.0, 60.0);
    bad.net_assimilation_umol_m2_s = 0.0;
    bad.hydraulic_beta = 0.0;
    bad.realized_water_mol = 0.0;
    cannaville::consequences::integrate_segment(stressed, stressed_view, p, bad);
    PlantConsequenceState good_state;
    PlantConsequencePresentation good_view;
    cannaville::consequences::integrate_segment(good_state, good_view, p, good(0.0, 60.0));
    auto recovery = good(60.0, 60.0);
    cannaville::consequences::integrate_segment(stressed, stressed_view, p, recovery);
    cannaville::consequences::integrate_segment(good_state, good_view, p, recovery);
    require(stressed.lost_opportunity_s > good_state.lost_opportunity_s, "history was lost");
    require(stressed.water_damage_0_1 >= good_state.water_damage_0_1, "damage ordering");
    PlantConsequenceState repeat_a;
    PlantConsequencePresentation repeat_view_a;
    PlantConsequenceState repeat_b;
    PlantConsequencePresentation repeat_view_b;
    cannaville::consequences::integrate_segment(repeat_a, repeat_view_a, p, bad);
    cannaville::consequences::integrate_segment(repeat_b, repeat_view_b, p, bad);
    require_near(repeat_a.lost_opportunity_s, repeat_b.lost_opportunity_s, 0.0, "repeat nondeterminism");
    require(repeat_view_a.derived_condition == repeat_view_b.derived_condition, "condition nondeterminism");
}

void test_unavailable_inputs_and_reason_codes() {
    PlantConsequenceState state;
    PlantConsequencePresentation presentation;
    auto input = good(0.0, 10.0);
    input.net_assimilation_umol_m2_s.reset();
    input.water_allocation_available = false;
    input.hydraulic_beta.reset();
    input.leaf_temperature_c.reset();
    input.absorbed_shortwave_w_m2.reset();
    cannaville::consequences::integrate_segment(state, presentation, profile(), input);
    require(state.integration_status == "opportunity_unavailable", "unavailable opportunity status");
    require_near(state.potential_opportunity_s, 0.0, 1e-12, "unavailable opportunity was integrated");
    require_near(state.achieved_opportunity_s, 0.0, 1e-12, "unavailable achieved opportunity was integrated");
    require_near(state.lost_opportunity_s, 0.0, 1e-12, "unavailable lost opportunity was fabricated");
    require(presentation.derived_condition == "UNAVAILABLE", "unavailable condition");
    bool found = false;
    for (const auto& reason : presentation.reason_codes) if (reason.reason_code == "P2_INPUT_UNAVAILABLE") found = true;
    require(found, "unavailable reason code");
}

void test_presentation_condition_and_profile_validation() {
    const auto p = profile();
    PlantConsequenceState healthy_state;
    PlantConsequencePresentation healthy_view;
    cannaville::consequences::integrate_segment(healthy_state, healthy_view, p, good(0.0, 10.0));
    require(healthy_view.derived_condition == "THRIVING", "healthy derived condition");
    require(healthy_view.reason_codes.empty(), "healthy plant emitted a cause");

    auto invalid = p;
    std::swap(invalid.stages[0], invalid.stages[1]);
    require(!cannaville::consequences::validate_profile(invalid).empty(), "stage order was not validated");
    invalid = p;
    invalid.presentation.critical_onset = 2.0;
    require(!cannaville::consequences::validate_profile(invalid).empty(), "presentation threshold was not validated");
}

void test_save_state_shape_is_not_health() {
    const auto p = profile();
    require(p.profile_id == "synthetic_p2_test", "test profile classification");
    require(p.evidence_classification == "SYNTHETIC_TEST_VALUE", "synthetic classification");
}

cannaville::core::Scenario simulation_scenario(double timestep_seconds) {
    using namespace cannaville;
    core::Scenario scenario;
    scenario.schema_version = 1;
    scenario.scenario_id = "p2-integration-test";
    scenario.simulation_version = "p2-test";
    scenario.fixed_timestep = units::Seconds{timestep_seconds};

    core::ScenarioRoomDefinition room;
    room.id = "room-a";
    room.width = units::Meters{1.0};
    room.depth = units::Meters{1.0};
    room.cell_size = units::Meters{1.0};
    core::ScenarioCellDefinition cell;
    cell.id = "cell-a";
    cell.center_x = units::Meters{0.5};
    cell.center_y = units::Meters{0.5};
    cell.environment.air_temperature = units::Celsius{25.0};
    cell.environment.relative_humidity = units::RelativeHumidityPercent{50.0};
    cell.environment.atmospheric_pressure = units::AtmosphericPressureKPa{101.325};
    cell.environment.carbon_dioxide = units::CO2MicromolesPerMole{450.0};
    cell.environment.airflow = units::AirflowMetersPerSecond{1.0};
    cell.environment.leaf_temperature = units::Celsius{25.0};
    cell.environment.incident_shortwave_w_m2 = 100.0;
    cell.environment.radiative_surroundings_temperature = units::Celsius{25.0};
    environment::derive_physical_state(cell.environment);
    cell.lighting_schedule.segments.push_back(lighting::LightingScheduleSegment{
        units::Seconds{0.0}, units::Seconds{86400.0}, units::PPFDMicromolesPerSquareMeterSecond{1000.0}, 100.0});
    room.cells.push_back(cell);
    scenario.rooms.push_back(room);

    core::ScenarioRootZoneDefinition root_zone;
    root_zone.id = "rootzone_plant-a";
    root_zone.type = "Substrate";
    root_zone.substrate_bulk_volume = units::VolumeCubicMeters{0.01};
    root_zone.initial_water_volume = units::VolumeCubicMeters{0.005};
    root_zone.substrate_hydraulic_profile_id = "synthetic_test_a";
    root_zone.hydraulic_stress_transfer_profile_id = "synthetic_stress_test";
    scenario.root_zones.push_back(root_zone);

    core::ScenarioPlantDefinition plant;
    plant.id = "plant-a";
    plant.room_id = "room-a";
    plant.cultivar_id = "synthetic-cultivar";
    plant.root_zone_id = root_zone.id;
    plant.x = units::Meters{0.5};
    plant.y = units::Meters{0.5};
    plant.z = units::Meters{0.0};
    plant.gas_exchange_profile_id = "synthetic_vegetative_test";
    plant.effective_transpiring_leaf_area_m2 = 1.0;
    plant.leaf_characteristic_dimension_m = 0.05;
    plant.leaf_energy_profile = leafenergy::get_synthetic_test_profile();
    plant.consequence_profile = profile();
    scenario.plants.push_back(plant);
    return scenario;
}

void test_timestep_invariance() {
    const auto p = profile();
    PlantConsequenceState one_step;
    PlantConsequencePresentation one_view;
    auto one = good(0.0, 60.0);
    cannaville::consequences::integrate_segment(one_step, one_view, p, one);
    PlantConsequenceState two_steps;
    PlantConsequencePresentation two_view;
    auto first = good(0.0, 30.0);
    auto second = good(30.0, 30.0);
    cannaville::consequences::integrate_segment(two_steps, two_view, p, first);
    cannaville::consequences::integrate_segment(two_steps, two_view, p, second);
    require_near(one_step.potential_opportunity_s, two_steps.potential_opportunity_s, 1e-12, "potential timestep dependence");
    require_near(one_step.achieved_opportunity_s, two_steps.achieved_opportunity_s, 1e-12, "achieved timestep dependence");
    require_near(one_step.lost_opportunity_s, two_steps.lost_opportunity_s, 1e-12, "lost timestep dependence");
    require(one_step.stage == two_steps.stage, "stage timestep dependence");
    require_near(one_step.stage_progress_0_1, two_steps.stage_progress_0_1, 1e-12, "progress timestep dependence");
}

void test_multi_day_management_trajectories() {
    auto p = profile();
    for (auto& stage : p.stages) stage.required_opportunity_s = 2000000.0;
    p.water.damage_scale_s = 3000000.0;
    p.thermal.damage_scale_s = 3000000.0;
    p.radiation.damage_scale_s = 3000000.0;

    auto favorable = good(0.0, 86400.0);
    auto temporary = favorable;
    temporary.net_assimilation_umol_m2_s = 0.0;
    temporary.hydraulic_beta = 0.0;
    temporary.realized_water_mol = 0.0;
    temporary.leaf_temperature_c = 40.0;
    temporary.absorbed_shortwave_w_m2 = 1000.0;
    auto chronic = temporary;

    PlantConsequenceState good_state;
    PlantConsequencePresentation good_view;
    PlantConsequenceState temporary_state;
    PlantConsequencePresentation temporary_view;
    PlantConsequenceState chronic_state;
    PlantConsequencePresentation chronic_view;

    cannaville::consequences::integrate_segment(good_state, good_view, p, favorable);
    cannaville::consequences::integrate_segment(temporary_state, temporary_view, p, temporary);
    cannaville::consequences::integrate_segment(chronic_state, chronic_view, p, chronic);
    for (int day = 1; day < 10; ++day) {
        auto good_day = favorable;
        good_day.start_time_s = static_cast<double>(day) * 86400.0;
        cannaville::consequences::integrate_segment(good_state, good_view, p, good_day);

        auto temporary_day = favorable;
        temporary_day.start_time_s = static_cast<double>(day) * 86400.0;
        cannaville::consequences::integrate_segment(temporary_state, temporary_view, p, temporary_day);

        auto chronic_day = chronic;
        chronic_day.start_time_s = static_cast<double>(day) * 86400.0;
        cannaville::consequences::integrate_segment(chronic_state, chronic_view, p, chronic_day);
    }

    require(good_state.achieved_opportunity_s > temporary_state.achieved_opportunity_s &&
            temporary_state.achieved_opportunity_s > chronic_state.achieved_opportunity_s,
            "trajectory achieved-opportunity ordering");
    require(good_state.stage_progress_0_1 > temporary_state.stage_progress_0_1 &&
            temporary_state.stage_progress_0_1 > chronic_state.stage_progress_0_1,
            "trajectory stage ordering");
    require(good_state.lost_opportunity_s < temporary_state.lost_opportunity_s &&
            temporary_state.lost_opportunity_s < chronic_state.lost_opportunity_s,
            "trajectory lost-opportunity ordering");
    require(good_state.water_damage_0_1 < temporary_state.water_damage_0_1 &&
            temporary_state.water_damage_0_1 < chronic_state.water_damage_0_1,
            "trajectory persistent-damage ordering");
    require(temporary_state.water_recovery_0_1 < 0.1, "temporary trajectory did not recover current water consequence");
    require(temporary_state.lost_opportunity_s > 0.0, "temporary trajectory erased history");
}

void test_exact_p1_segmentation_reaches_p2() {
    auto lighting_scenario = simulation_scenario(900.0);
    auto& lighting_schedule = lighting_scenario.rooms.front().cells.front().lighting_schedule;
    lighting_schedule.segments.clear();
    lighting_schedule.segments.push_back({cannaville::units::Seconds{0.0}, cannaville::units::Seconds{450.0},
                                          cannaville::units::PPFDMicromolesPerSquareMeterSecond{0.0}, 0.0});
    lighting_schedule.segments.push_back({cannaville::units::Seconds{450.0}, cannaville::units::Seconds{86400.0},
                                          cannaville::units::PPFDMicromolesPerSquareMeterSecond{1000.0}, 100.0});
    cannaville::core::Simulation lighting_simulation(lighting_scenario, 7);
    lighting_simulation.advance_fixed_step();
    const auto& light_trace = lighting_simulation.last_step_segment_trace();
    require(light_trace.size() == 2, "P2 light segmentation count");
    require_near(light_trace[0].duration_s, 450.0, 1e-12, "P2 light segment one duration");
    require_near(light_trace[1].duration_s, 450.0, 1e-12, "P2 light segment two duration");
    require_near(light_trace[0].ppfd_umol_m2_s, 0.0, 1e-12, "P2 light segment one PPFD");
    require_near(light_trace[1].ppfd_umol_m2_s, 1000.0, 1e-12, "P2 light segment two PPFD");
    require(light_trace[0].consequence_status != "" && light_trace[1].consequence_status != "", "P2 did not consume light segments");
    require(light_trace[0].opportunity_signal_0_1 != light_trace[1].opportunity_signal_0_1, "P2 light segment opportunities did not differ");

    auto water_scenario = simulation_scenario(900.0);
    water_scenario.water_events.push_back({"midstep-irrigation", cannaville::units::Seconds{420.0},
                                           "rootzone_plant-a", "irrigation", cannaville::units::VolumeCubicMeters{0.004}});
    cannaville::core::Simulation water_simulation(water_scenario, 7);
    water_simulation.advance_fixed_step();
    const auto& water_trace = water_simulation.last_step_segment_trace();
    require(water_trace.size() == 2, "P2 water segmentation count");
    require_near(water_trace[0].duration_s, 420.0, 1e-12, "P2 water segment one duration");
    require_near(water_trace[1].duration_s, 480.0, 1e-12, "P2 water segment two duration");
    require(water_trace[0].consequence_status != "" && water_trace[1].consequence_status != "", "P2 did not consume water segments");
    require_near(water_trace[0].realized_water_m3 + water_trace[1].realized_water_m3,
                 water_simulation.full_state_for_internal_use().root_zones.front().cumulative_realized_withdrawal.value,
                 1e-12, "P2 segment water withdrawals do not reconcile");
}

void test_simulation_integration_csv_and_save_load() {
    const auto scenario = simulation_scenario(60.0);
    cannaville::core::Simulation original(scenario, 7);
    original.advance_fixed_step();
    const auto& plant = original.full_state_for_internal_use().plants.front();
    require(plant.consequence_profile.is_configured, "simulation did not retain consequence profile");
    require(plant.consequence_state.potential_opportunity_s > 0.0, "simulation did not integrate P2");
    require(original.plant_physiology_csv_header().find("lost_opportunity_s") != std::string::npos, "P2 CSV column missing");
    require(original.plant_physiology_csv_header().find("reason_codes") != std::string::npos, "P2 reason CSV column missing");
    const std::string saved = original.serialize_state();
    require(saved.find("\"health\"") == std::string::npos, "generic health became serialized source of truth");
    cannaville::core::Simulation restored;
    restored.load_serialized_state(saved);
    require(restored.serialize_state() == saved, "P2 state did not round-trip");
    original.advance_fixed_step();
    restored.advance_fixed_step();
    require(original.serialize_state() == restored.serialize_state(), "P2 save/load continuation diverged");
    require(original.plant_physiology_csv_row() == restored.plant_physiology_csv_row(), "P2 CSV continuation diverged");
}

} // namespace

int main() {
    try {
        test_opportunity_arithmetic_and_monotonic_loss();
        test_lifecycle_and_exact_transition_remainder();
        test_explicit_stage_gates();
        test_water_thermal_radiation_and_recovery();
        test_history_dependence_and_determinism();
        test_unavailable_inputs_and_reason_codes();
        test_presentation_condition_and_profile_validation();
        test_save_state_shape_is_not_health();
        test_timestep_invariance();
        test_multi_day_management_trajectories();
        test_exact_p1_segmentation_reaches_p2();
        test_simulation_integration_csv_and_save_load();
        std::cout << "P2 tests passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "P2 test failure: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
