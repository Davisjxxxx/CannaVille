#include "Simulation/Core/Scenario.hpp"
#include "Simulation/Core/Simulation.hpp"
#include "Simulation/Environment/VaporPressure.hpp"
#include "Simulation/GasExchange/GasExchange.hpp"
#include "Simulation/LeafEnergy/LeafEnergy.hpp"
#include "Simulation/Transpiration/Transpiration.hpp"

#include <cmath>
#include <iostream>
#include <optional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

using cannaville::environment::EnvironmentState;
using cannaville::gasexchange::CalibrationProfile;
using cannaville::leafenergy::LeafEnergyEvaluation;
using cannaville::leafenergy::LeafEnergyProfile;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void require_near(double actual, double expected, double tolerance, const std::string& message) {
    if (std::abs(actual - expected) > tolerance) {
        std::ostringstream out;
        out << message << " actual=" << actual << " expected=" << expected;
        throw std::runtime_error(out.str());
    }
}

EnvironmentState environment(double shortwave = 500.0, double radiative = 25.0, double airflow = 1.0) {
    EnvironmentState value;
    value.air_temperature.value = 25.0;
    value.relative_humidity.value = 50.0;
    value.atmospheric_pressure.value = 101.325;
    value.carbon_dioxide.value = 400.0;
    value.airflow = cannaville::units::AirflowMetersPerSecond{airflow};
    value.incident_shortwave_w_m2 = shortwave;
    value.radiative_surroundings_temperature = cannaville::units::Celsius{radiative};
    cannaville::environment::derive_physical_state(value);
    return value;
}

cannaville::lighting::LightingState lighting(double ppfd = 1000.0) {
    cannaville::lighting::LightingState value;
    value.ppfd.value = ppfd;
    value.light_on = ppfd > 0.0;
    return value;
}

LeafEnergyEvaluation evaluate(const EnvironmentState& env, const LeafEnergyProfile& profile,
                              std::optional<double> beta = 1.0) {
    return cannaville::leafenergy::evaluate_leaf_energy_residual(
        cannaville::units::Celsius{25.0}, env, lighting(), profile,
        cannaville::gasexchange::get_synthetic_vegetative_test_profile(), beta, 0.1);
}

LeafEnergyProfile solved_profile() {
    return cannaville::leafenergy::get_synthetic_test_profile();
}

void require_solved(const LeafEnergyProfile& profile, const EnvironmentState& env,
                    double beta, double& temperature, std::string label) {
    cannaville::leafenergy::LeafEnergyState state;
    cannaville::leafenergy::solve_leaf_energy_balance(
        state, profile, env, lighting(),
        cannaville::gasexchange::get_synthetic_vegetative_test_profile(), beta, 0.1);
    require(state.status == "converged", label + " did not converge: " + state.status);
    require(state.leaf_temperature.has_value(), label + " has no leaf temperature");
    require(state.energy_residual_w_m2.has_value(), label + " has no residual");
    require(std::abs(*state.energy_residual_w_m2) <= profile.residual_tolerance_w_m2,
            label + " residual exceeds tolerance");
    temperature = state.leaf_temperature->value;
}

void test_area_basis_and_analytic_equilibrium() {
    LeafEnergyProfile profile = solved_profile();
    profile.shortwave_absorptance = 0.0;
    profile.solver_min_temperature_c = 25.0;
    profile.solver_max_temperature_c = 25.01;
    EnvironmentState env = environment(0.0, 25.0, 1.0);
    env.relative_humidity.value = 100.0;
    cannaville::environment::derive_physical_state(env);
    double temperature = 0.0;
    require_solved(profile, env, 1.0, temperature, "analytic equilibrium");
    require_near(temperature, 25.0, 0.001, "analytic equilibrium temperature");

    EnvironmentState normal_env = environment();
    LeafEnergyProfile one_sided = solved_profile();
    LeafEnergyProfile two_surface = one_sided;
    two_surface.surface_factor = 2.0;
    const LeafEnergyEvaluation a = evaluate(normal_env, one_sided);
    const LeafEnergyEvaluation b = evaluate(normal_env, two_surface);
    require(a.available && b.available, "area-basis evaluations unavailable");
    require_near(a.latent_heat_flux_w_m2, b.latent_heat_flux_w_m2, 1e-12,
                 "surface factor double-counted latent heat");
    require_near(a.transpiration.flux_mol_m2_s, b.transpiration.flux_mol_m2_s, 1e-12,
                 "surface factor double-counted water flux");
    const LeafEnergyEvaluation warmer_trial = cannaville::leafenergy::evaluate_leaf_energy_residual(
        cannaville::units::Celsius{30.0}, normal_env, lighting(), one_sided,
        cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    const LeafEnergyEvaluation cooler_trial = cannaville::leafenergy::evaluate_leaf_energy_residual(
        cannaville::units::Celsius{20.0}, normal_env, lighting(), one_sided,
        cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    require(warmer_trial.available && cooler_trial.available, "sensible-heat sign cases unavailable");
    require(warmer_trial.sensible_heat_flux_w_m2 > 0.0 && cooler_trial.sensible_heat_flux_w_m2 < 0.0,
            "sensible-heat sign convention");
    std::cout << "ANALYTIC temperature_c=" << temperature << " residual_w_m2="
              << *([](const LeafEnergyProfile& p, const EnvironmentState& e) {
                     cannaville::leafenergy::LeafEnergyState s;
                     cannaville::leafenergy::solve_leaf_energy_balance(
                         s, p, e, lighting(), cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
                     return s.energy_residual_w_m2;
                 }(profile, env)) << "\n";
}

void test_causal_monotonicity_and_reference_case() {
    const LeafEnergyProfile profile = solved_profile();
    double low_light = 0.0;
    double high_light = 0.0;
    require_solved(profile, environment(100.0), 1.0, low_light, "low shortwave");
    require_solved(profile, environment(700.0), 1.0, high_light, "high shortwave");
    require(high_light > low_light, "higher absorbed shortwave did not raise leaf temperature");

    double low_airflow = 0.0;
    double high_airflow = 0.0;
    require_solved(profile, environment(500.0, 25.0, 0.05), 1.0, low_airflow, "low airflow");
    require_solved(profile, environment(500.0, 25.0, 2.0), 1.0, high_airflow, "high airflow");
    require(std::abs(high_airflow - 25.0) < std::abs(low_airflow - 25.0),
            "higher airflow did not reduce leaf-air divergence");

    const LeafEnergyEvaluation low_transpiration = evaluate(environment(), profile, 0.2);
    const LeafEnergyEvaluation high_transpiration = evaluate(environment(), profile, 1.0);
    require(low_transpiration.available && high_transpiration.available, "transpiration comparison unavailable");
    require(high_transpiration.latent_heat_flux_w_m2 > low_transpiration.latent_heat_flux_w_m2,
            "higher transpiration did not increase latent cooling");

    cannaville::leafenergy::LeafEnergyState repeat_a;
    cannaville::leafenergy::LeafEnergyState repeat_b;
    cannaville::leafenergy::solve_leaf_energy_balance(
        repeat_a, profile, environment(), lighting(), cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    cannaville::leafenergy::solve_leaf_energy_balance(
        repeat_b, profile, environment(), lighting(), cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    require(repeat_a.status == repeat_b.status && repeat_a.leaf_temperature.has_value() == repeat_b.leaf_temperature.has_value() &&
                repeat_a.leaf_temperature.has_value() && repeat_a.leaf_temperature->value == repeat_b.leaf_temperature->value &&
                repeat_a.energy_residual_w_m2.has_value() == repeat_b.energy_residual_w_m2.has_value() &&
                repeat_a.energy_residual_w_m2.has_value() && *repeat_a.energy_residual_w_m2 == *repeat_b.energy_residual_w_m2,
            "repeated leaf-energy solve was not deterministic");

    double limited = 0.0;
    double unrestricted = 0.0;
    require_solved(profile, environment(), 0.2, limited, "hydraulic limitation");
    require_solved(profile, environment(), 1.0, unrestricted, "unrestricted hydraulics");
    require(limited > unrestricted, "hydraulic limitation did not raise leaf temperature");

    double cool_surroundings = 0.0;
    double warm_surroundings = 0.0;
    require_solved(profile, environment(500.0, 15.0), 1.0, cool_surroundings, "cool radiative surroundings");
    require_solved(profile, environment(500.0, 35.0), 1.0, warm_surroundings, "warm radiative surroundings");
    require(warm_surroundings > cool_surroundings, "warmer surroundings did not raise leaf temperature");

    // Scoped published/example input: tealeaves-style single-leaf forcing. This is
    // a reference-case input, not a calibrated production profile or target value.
    double published_example = 0.0;
    require_solved(profile, environment(1000.0, 25.0, 2.0), 1.0, published_example, "published example input");
    std::cout << "REFERENCE published_example_leaf_temperature_c=" << published_example
              << " low_light_c=" << low_light << " high_light_c=" << high_light
              << " low_airflow_c=" << low_airflow << " high_airflow_c=" << high_airflow
              << " low_beta_c=" << limited << " high_beta_c=" << unrestricted
              << " cool_surroundings_c=" << cool_surroundings
              << " warm_surroundings_c=" << warm_surroundings << "\n";
}

void test_missing_and_failure_statuses() {
    LeafEnergyProfile profile = solved_profile();
    EnvironmentState env = environment();
    auto result = evaluate(env, profile);
    require(result.available, "baseline residual unavailable");

    env.incident_shortwave_w_m2.reset();
    require(evaluate(env, profile).status == "missing_incident_shortwave", "missing shortwave status");
    cannaville::leafenergy::LeafEnergyState ppfd_only_state;
    cannaville::leafenergy::solve_leaf_energy_balance(
        ppfd_only_state, profile, env, lighting(), cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    require(ppfd_only_state.status == "missing_incident_shortwave", "PPFD-only solved mode activated");
    env = environment();
    env.radiative_surroundings_temperature.reset();
    require(evaluate(env, profile).status == "missing_radiative_surroundings_temperature", "missing radiative status");
    env = environment(500.0, 25.0, 1000.0);
    require(evaluate(env, profile).status == "unsupported_boundary_layer", "unsupported boundary status");

    env = environment(1.0e6);
    cannaville::leafenergy::LeafEnergyState state;
    cannaville::leafenergy::solve_leaf_energy_balance(
        state, profile, env, lighting(), cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    require(state.status == "solver_no_bracket", "no-bracket status");

    profile = solved_profile();
    profile.max_iterations = 1;
    profile.temperature_tolerance_k = 1e-12;
    profile.residual_tolerance_w_m2 = 1e-12;
    state = {};
    cannaville::leafenergy::solve_leaf_energy_balance(
        state, profile, environment(), lighting(), cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    require(state.status == "solver_iteration_limit", "iteration-limit status");

    profile = solved_profile();
    profile.temperature_mode = cannaville::leafenergy::LeafTemperatureMode::ExternalLeafTemperature;
    env = environment();
    env.leaf_temperature = cannaville::units::Celsius{24.0};
    env.leaf_vpd = cannaville::units::VPDKPa{1.2};
    cannaville::leafenergy::solve_leaf_energy_balance(
        state, profile, env, lighting(), cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    require(state.status == "external_temperature_used", "external mode status");
    require_near(state.leaf_temperature->value, 24.0, 1e-12, "external temperature overwritten");

    env = environment();
    env.relative_humidity.value = 100.0;
    cannaville::environment::derive_physical_state(env);
    const auto boundary = cannaville::transpiration::calculate_boundary_layer_conductance(
        cannaville::units::AirflowMetersPerSecond{1.0}, cannaville::units::Meters{0.1},
        cannaville::transpiration::calculate_air_properties(env.air_temperature, env.atmospheric_pressure),
        cannaville::units::Celsius{20.0}, env.air_temperature);
    const auto condensation = cannaville::transpiration::calculate_transpiration(
        cannaville::units::StomatalConductanceMolesPerSquareMeterSecond{0.2}, boundary,
        cannaville::units::VPDKPa{-0.2}, env.atmospheric_pressure);
    require(condensation.available && condensation.flux_mol_m2_s < 0.0, "condensation was not retained");
    require(condensation.status == "condensation", "condensation status");

    LeafEnergyProfile non_finite_profile = solved_profile();
    non_finite_profile.surface_factor = std::numeric_limits<double>::max();
    const auto non_finite = cannaville::leafenergy::evaluate_leaf_energy_residual(
        cannaville::units::Celsius{30.0}, environment(), lighting(), non_finite_profile,
        cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    require(non_finite.status == "non_finite_residual", "non-finite residual status");

    EnvironmentState invalid_pressure = environment();
    invalid_pressure.atmospheric_pressure.value = 0.0;
    cannaville::leafenergy::LeafEnergyState invalid_state;
    cannaville::leafenergy::solve_leaf_energy_balance(
        invalid_state, solved_profile(), invalid_pressure, lighting(),
        cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    require(invalid_state.status == "invalid_pressure", "invalid pressure status");
    LeafEnergyProfile invalid_domain = solved_profile();
    invalid_domain.solver_min_temperature_c = 60.0;
    invalid_domain.solver_max_temperature_c = 70.0;
    invalid_state = {};
    cannaville::leafenergy::solve_leaf_energy_balance(
        invalid_state, invalid_domain, environment(), lighting(),
        cannaville::gasexchange::get_synthetic_vegetative_test_profile(), 1.0, 0.1);
    require(invalid_state.status == "invalid_temperature_domain", "invalid temperature domain status");
    std::cout << "FAILURE_STATUSES no_bracket=solver_no_bracket iteration_limit=solver_iteration_limit"
              << " unsupported_boundary=unsupported_boundary_layer condensation_flux="
              << condensation.flux_mol_m2_s << "\n";
}

std::string scenario_json() {
    return R"JSON({
      "schema_version":1,"scenario_id":"p1b5-test","simulation_version":"p1b5-test-v1",
      "fixed_timestep_seconds":900,
      "rooms":[{"id":"room","width_m":2,"depth_m":2,"cell_size_m":2,"cells":[
        {"id":"cell","center_x_m":1,"center_y_m":1,
         "environment":{"air_temperature_c":25,"relative_humidity_percent":50,"atmospheric_pressure_kpa":101.325,"co2_umol_per_mol":400,"airflow_m_per_s":1,"incident_shortwave_w_m2":500,"radiative_surroundings_temperature_c":25},
         "lighting_schedule":[{"start_seconds":0,"end_seconds":450,"ppfd_umol_per_m2_s":0,"incident_shortwave_w_m2":0},{"start_seconds":450,"end_seconds":86400,"ppfd_umol_per_m2_s":1000,"incident_shortwave_w_m2":500}]}]}],
      "root_zones":[{"id":"rz","type":"Reservoir","max_stored_water_m3":0.01,"initial_water_volume_m3":0.01,"explicit_unrestricted_water_access":true}],
      "plants":[{"id":"p","room_id":"room","cultivar_id":"synthetic","root_zone_id":"rz","x_m":1,"y_m":1,"z_m":0,"gas_exchange_profile_id":"synthetic_vegetative_test","effective_transpiring_leaf_area_m2":1,"leaf_characteristic_dimension_m":0.1}]
    })JSON";
}

void configure_solved(cannaville::core::Scenario& scenario) {
    scenario.plants.front().leaf_energy_profile = solved_profile();
    scenario.plants.front().leaf_energy_profile.temperature_mode = cannaville::leafenergy::LeafTemperatureMode::SolvedLeafEnergyBalance;
}

void test_segmenting_and_serialization() {
    auto scenario = cannaville::core::Scenario::load_json(scenario_json());
    configure_solved(scenario);
    cannaville::core::Simulation simulation(scenario, 42);
    simulation.advance_fixed_step();
    const auto& trace = simulation.last_step_segment_trace();
    require(trace.size() == 2, "expected exact light segmentation");
    require(trace[0].duration_s == 450.0 && trace[1].duration_s == 450.0, "wrong light segment durations");
    require(trace[0].ppfd_umol_m2_s == 0.0 && trace[1].ppfd_umol_m2_s == 1000.0, "wrong light segment PPFD");
    require(trace[0].leaf_energy_status == "converged" && trace[1].leaf_energy_status == "converged",
            "light segment solve missing: " + trace[0].leaf_energy_status + "," + trace[1].leaf_energy_status +
                " gas=" + trace[0].gas_exchange_status + "," + trace[1].gas_exchange_status);
    require(trace[0].leaf_temperature_c != trace[1].leaf_temperature_c, "light segment solutions did not differ");
    require_near(trace[0].requested_water_m3 + trace[1].requested_water_m3,
                 simulation.full_state_for_internal_use().root_zones.front().cumulative_realized_withdrawal.value +
                     simulation.full_state_for_internal_use().root_zones.front().cumulative_unmet_demand.value,
                 1e-15, "light segment water integration");
    std::cout << "SEGMENT light_1_ppfd=" << trace[0].ppfd_umol_m2_s << " light_2_ppfd=" << trace[1].ppfd_umol_m2_s
              << " light_1_leaf_c=" << *trace[0].leaf_temperature_c << " light_2_leaf_c=" << *trace[1].leaf_temperature_c
              << "\n";

    const std::string saved = simulation.serialize_state();
    cannaville::core::Simulation restored(scenario, 42);
    restored.load_serialized_state(saved);
    require(restored.serialize_state() == saved, "leaf energy serialization round trip");
    simulation.advance_fixed_step();
    restored.advance_fixed_step();
    require(simulation.serialize_state() == restored.serialize_state(), "leaf energy save/load continuation");
    auto water_scenario = cannaville::core::Scenario::load_json(scenario_json());
    configure_solved(water_scenario);
    water_scenario.water_events.push_back({"midstep-topoff", cannaville::units::Seconds{450.0}, "rz", "top_off",
                                            cannaville::units::VolumeCubicMeters{0.001}});
    cannaville::core::Simulation water_simulation(water_scenario, 42);
    water_simulation.advance_fixed_step();
    const auto& water_trace = water_simulation.last_step_segment_trace();
    require(water_trace.size() == 2, "expected exact water-event segmentation");
    require(water_trace[0].duration_s == 450.0 && water_trace[1].duration_s == 450.0,
            "wrong water-event segment durations");
    require_near(water_trace[0].water_input_before_segment_m3, 0.0, 1e-15, "water event applied to first segment");
    require_near(water_trace[1].water_input_before_segment_m3, 0.001, 1e-15, "water event missing from second segment");
    std::cout << "SEGMENT water_1_input_m3=" << water_trace[0].water_input_before_segment_m3
              << " water_2_input_m3=" << water_trace[1].water_input_before_segment_m3 << "\n";
}

} // namespace

int main() {
    try {
        test_area_basis_and_analytic_equilibrium();
        test_causal_monotonicity_and_reference_case();
        test_missing_and_failure_statuses();
        test_segmenting_and_serialization();
        std::cout << "P1B5_TESTS_PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "P1B5_TEST_FAILURE: " << error.what() << "\n";
        return 1;
    }
}
