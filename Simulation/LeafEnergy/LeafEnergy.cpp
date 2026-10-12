#include "Simulation/LeafEnergy/LeafEnergy.hpp"

#include "Simulation/Environment/VaporPressure.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace cannaville::leafenergy {

namespace {

constexpr double kStefanBoltzmannWm2K4 = 5.670374419e-8;
constexpr double kWaterMolarMassKgMol = 0.01801528;
constexpr double kMinimumSupportedVaporPressureTemperatureC = 0.0;
constexpr double kMaximumSupportedVaporPressureTemperatureC = 50.0;

bool finite(double value) {
    return std::isfinite(value);
}

void clear_state(LeafEnergyState& state, const LeafEnergyProfile& profile) {
    state = LeafEnergyState{};
    state.model_version = profile.model_version;
}

void populate_state(LeafEnergyState& state, const LeafEnergyEvaluation& evaluation,
                    const std::string& status, const std::string& source) {
    state.status = status;
    state.source = source;
    state.solver_iterations = evaluation.iterations;
    if (!evaluation.available) return;
    state.leaf_temperature = evaluation.trial_leaf_temperature;
    state.leaf_vpd = evaluation.leaf_vpd;
    state.absorbed_shortwave_w_m2 = evaluation.absorbed_shortwave_w_m2;
    state.net_longwave_w_m2 = evaluation.net_longwave_w_m2;
    state.sensible_heat_flux_w_m2 = evaluation.sensible_heat_flux_w_m2;
    state.latent_heat_flux_w_m2 = evaluation.latent_heat_flux_w_m2;
    state.energy_residual_w_m2 = evaluation.energy_residual_w_m2;
}

bool valid_profile(const LeafEnergyProfile& profile) {
    return profile.is_configured && !profile.profile_id.empty() &&
        finite(profile.shortwave_absorptance) && profile.shortwave_absorptance >= 0.0 &&
        profile.shortwave_absorptance <= 1.0 && finite(profile.longwave_emissivity) &&
        profile.longwave_emissivity > 0.0 && profile.longwave_emissivity <= 1.0 &&
        finite(profile.surface_factor) && profile.surface_factor > 0.0 &&
        profile.area_basis == AreaBasis::ProjectedOneSidedLeafArea &&
        finite(profile.solver_min_temperature_c) && finite(profile.solver_max_temperature_c) &&
        profile.solver_min_temperature_c < profile.solver_max_temperature_c &&
        finite(profile.air_relative_bracket_k) && profile.air_relative_bracket_k > 0.0 &&
        finite(profile.temperature_tolerance_k) && profile.temperature_tolerance_k > 0.0 &&
        finite(profile.residual_tolerance_w_m2) && profile.residual_tolerance_w_m2 > 0.0 &&
        profile.max_iterations > 0;
}

std::string boundary_status(transpiration::ScientificDomainStatus status) {
    switch (status) {
        case transpiration::ScientificDomainStatus::UnsupportedFlowRegime:
            return "unsupported_boundary_layer";
        case transpiration::ScientificDomainStatus::InvalidDimension:
            return "missing_leaf_characteristic_dimension";
        case transpiration::ScientificDomainStatus::Valid:
            return "valid";
    }
    return "unsupported_boundary_layer";
}

void assign_final(LeafEnergyState& state, const LeafEnergyEvaluation& evaluation,
                  std::uint32_t iterations) {
    LeafEnergyEvaluation final = evaluation;
    final.iterations = iterations;
    populate_state(state, final, "converged", "solved_leaf_energy_balance");
    state.solver_iterations = iterations;
}

double non_condensing_lower_bound(units::VaporPressureKPa actual_vapor_pressure) {
    // The accepted gas-exchange model has no authoritative negative-VPD
    // stomatal solution. Keep condensation available to the direct
    // transpiration/latent-heat path, but keep the coupled root search in the
    // non-condensing domain so a trial below the dew point cannot masquerade
    // as a solver failure.
    double low = kMinimumSupportedVaporPressureTemperatureC;
    double high = kMaximumSupportedVaporPressureTemperatureC;
    for (int iteration = 0; iteration < 48; ++iteration) {
        const double midpoint = low + ((high - low) * 0.5);
        if (environment::saturation_vapor_pressure_fao56(units::Celsius{midpoint}).value < actual_vapor_pressure.value) {
            low = midpoint;
        } else {
            high = midpoint;
        }
    }
    return high;
}

} // namespace

std::string to_string(AreaBasis basis) {
    switch (basis) {
        case AreaBasis::ProjectedOneSidedLeafArea:
            return "projected_one_sided_leaf_area";
    }
    return "unknown";
}

AreaBasis area_basis_from_string(const std::string& value) {
    if (value == "projected_one_sided_leaf_area") return AreaBasis::ProjectedOneSidedLeafArea;
    throw std::invalid_argument("unsupported leaf energy area_basis: " + value);
}

std::string to_string(LeafTemperatureMode mode) {
    switch (mode) {
        case LeafTemperatureMode::ExternalLeafTemperature: return "EXTERNAL_LEAF_TEMPERATURE";
        case LeafTemperatureMode::SolvedLeafEnergyBalance: return "SOLVED_LEAF_ENERGY_BALANCE";
        case LeafTemperatureMode::Unavailable: return "UNAVAILABLE";
    }
    return "UNAVAILABLE";
}

LeafTemperatureMode leaf_temperature_mode_from_string(const std::string& value) {
    if (value == "EXTERNAL_LEAF_TEMPERATURE") return LeafTemperatureMode::ExternalLeafTemperature;
    if (value == "SOLVED_LEAF_ENERGY_BALANCE") return LeafTemperatureMode::SolvedLeafEnergyBalance;
    if (value == "UNAVAILABLE") return LeafTemperatureMode::Unavailable;
    throw std::invalid_argument("unsupported leaf_temperature_mode: " + value);
}

LeafEnergyProfile get_unconfigured_profile() {
    LeafEnergyProfile profile;
    profile.profile_id = "unconfigured";
    profile.temperature_mode = LeafTemperatureMode::Unavailable;
    profile.source = "unconfigured";
    profile.evidence_classification = "CALIBRATION_REQUIRED";
    profile.calibration_status = "CALIBRATION_REQUIRED";
    return profile;
}

LeafEnergyProfile get_synthetic_test_profile() {
    LeafEnergyProfile profile;
    profile.profile_id = "synthetic_leaf_energy_test";
    profile.model_version = "leaf_energy_balance.steady_state.v1";
    profile.temperature_mode = LeafTemperatureMode::SolvedLeafEnergyBalance;
    profile.shortwave_absorptance = 0.5;
    profile.longwave_emissivity = 0.97;
    profile.surface_factor = 1.0;
    profile.area_basis = AreaBasis::ProjectedOneSidedLeafArea;
    profile.solver_min_temperature_c = -40.0;
    profile.solver_max_temperature_c = 100.0;
    profile.air_relative_bracket_k = 80.0;
    profile.temperature_tolerance_k = 0.001;
    profile.residual_tolerance_w_m2 = 0.01;
    profile.max_iterations = 32;
    profile.source = "synthetic_fixture";
    profile.evidence_classification = "SYNTHETIC_TEST_VALUE";
    profile.calibration_status = "SYNTHETIC_TEST_VALUE";
    profile.valid_domain = "engineering fixture; not biologically calibrated";
    profile.is_configured = true;
    return profile;
}

double latent_heat_of_vaporization_molar_j_mol(units::Celsius reference_temperature) {
    if (!finite(reference_temperature.value) || reference_temperature.value < 0.0 ||
        reference_temperature.value > 50.0) {
        throw std::invalid_argument("latent heat reference temperature requires 0 <= temperature_c <= 50");
    }
    const double lambda_mass_mj_kg = 2.501 - (0.002361 * reference_temperature.value);
    return lambda_mass_mj_kg * 1000000.0 * kWaterMolarMassKgMol;
}

LeafEnergyEvaluation evaluate_leaf_energy_residual(
    units::Celsius trial_leaf_temperature,
    const environment::EnvironmentState& environment,
    const lighting::LightingState& lighting,
    const LeafEnergyProfile& profile,
    const gasexchange::CalibrationProfile& gas_exchange_profile,
    std::optional<double> beta_hydraulic,
    std::optional<double> characteristic_dimension_m
) {
    LeafEnergyEvaluation result;
    result.trial_leaf_temperature = trial_leaf_temperature;

    if (!valid_profile(profile)) {
        result.status = "missing_leaf_energy_profile";
        return result;
    }
    if (!environment.incident_shortwave_w_m2.has_value()) {
        result.status = "missing_incident_shortwave";
        return result;
    }
    if (!environment.radiative_surroundings_temperature.has_value()) {
        result.status = "missing_radiative_surroundings_temperature";
        return result;
    }
    if (!environment.airflow.has_value()) {
        result.status = "missing_airflow";
        return result;
    }
    if (!characteristic_dimension_m.has_value() || !finite(*characteristic_dimension_m) ||
        *characteristic_dimension_m <= 0.0) {
        result.status = "missing_leaf_characteristic_dimension";
        return result;
    }
    if (!finite(environment.atmospheric_pressure.value) || environment.atmospheric_pressure.value <= 0.0) {
        result.status = "invalid_pressure";
        return result;
    }
    if (!finite(*environment.incident_shortwave_w_m2) || *environment.incident_shortwave_w_m2 < 0.0) {
        result.status = "missing_incident_shortwave";
        return result;
    }
    if (!finite(environment.radiative_surroundings_temperature->value)) {
        result.status = "missing_radiative_surroundings_temperature";
        return result;
    }
    if (!finite(trial_leaf_temperature.value) || trial_leaf_temperature.value < kMinimumSupportedVaporPressureTemperatureC ||
        trial_leaf_temperature.value > kMaximumSupportedVaporPressureTemperatureC) {
        result.status = "invalid_temperature_domain";
        return result;
    }

    (void)environment::saturation_vapor_pressure_fao56(trial_leaf_temperature);
    result.leaf_vpd = environment::leaf_vpd(trial_leaf_temperature, environment.actual_vapor_pressure);
    if (!result.leaf_vpd.has_value()) {
        result.status = "missing_leaf_vpd";
        return result;
    }
    if (result.leaf_vpd->value < 0.0) {
        result.status = "condensation";
        return result;
    }

    gasexchange::GasExchangeState gas_state;
    gasexchange::solve_coupled_gas_exchange(
        gas_state,
        gas_exchange_profile,
        lighting.ppfd,
        environment.carbon_dioxide,
        *result.leaf_vpd,
        trial_leaf_temperature,
        environment.atmospheric_pressure,
        beta_hydraulic);
    result.gas_exchange = gas_state;
    if (gas_state.status != gasexchange::ConvergenceStatus::Converged) {
        result.status = "gas_exchange_unavailable";
        return result;
    }

    const transpiration::AirProperties air_properties = transpiration::calculate_air_properties(
        environment.air_temperature, environment.atmospheric_pressure);
    result.boundary_layer = transpiration::calculate_boundary_layer_conductance(
        *environment.airflow,
        units::Meters{*characteristic_dimension_m},
        air_properties,
        trial_leaf_temperature,
        environment.air_temperature);
    if (!result.boundary_layer.value_mol_m2_s.has_value() ||
        !result.boundary_layer.value_heat_m_s.has_value()) {
        result.status = boundary_status(result.boundary_layer.status);
        return result;
    }

    result.transpiration = transpiration::calculate_transpiration(
        gas_state.stomatal_conductance,
        result.boundary_layer,
        *result.leaf_vpd,
        environment.atmospheric_pressure);
    if (!result.transpiration.available) {
        result.status = result.transpiration.status.empty() ? "transpiration_unavailable" : result.transpiration.status;
        return result;
    }

    const double t_leaf_k = trial_leaf_temperature.value + 273.15;
    const double t_surroundings_k = environment.radiative_surroundings_temperature->value + 273.15;
    result.absorbed_shortwave_w_m2 = profile.shortwave_absorptance * *environment.incident_shortwave_w_m2;
    result.net_longwave_w_m2 = profile.surface_factor * profile.longwave_emissivity * kStefanBoltzmannWm2K4 *
        (std::pow(t_surroundings_k, 4.0) - std::pow(t_leaf_k, 4.0));
    result.sensible_heat_flux_w_m2 = profile.surface_factor * *result.boundary_layer.value_heat_m_s *
        air_properties.air_density_kg_m3 * air_properties.cp_air_j_kg_k *
        (trial_leaf_temperature.value - environment.air_temperature.value);
    const double lambda_molar = latent_heat_of_vaporization_molar_j_mol(trial_leaf_temperature);
    result.latent_heat_flux_w_m2 = result.transpiration.flux_mol_m2_s * lambda_molar;
    result.energy_residual_w_m2 = result.absorbed_shortwave_w_m2 + result.net_longwave_w_m2 -
        result.sensible_heat_flux_w_m2 - result.latent_heat_flux_w_m2;
    if (!finite(result.energy_residual_w_m2)) {
        result.status = "non_finite_residual";
        return result;
    }
    result.available = true;
    result.status = "valid";
    return result;
}

void solve_leaf_energy_balance(
    LeafEnergyState& state,
    const LeafEnergyProfile& profile,
    const environment::EnvironmentState& environment,
    const lighting::LightingState& lighting,
    const gasexchange::CalibrationProfile& gas_exchange_profile,
    std::optional<double> beta_hydraulic,
    std::optional<double> characteristic_dimension_m
) {
    clear_state(state, profile);

    if (profile.temperature_mode == LeafTemperatureMode::Unavailable) {
        state.status = "unavailable";
        return;
    }
    if (profile.temperature_mode == LeafTemperatureMode::ExternalLeafTemperature) {
        if (!environment.leaf_temperature.has_value()) {
            state.status = "missing_leaf_temperature";
            return;
        }
        if (!environment.leaf_vpd.has_value()) {
            state.status = "missing_leaf_vpd";
            return;
        }
        state.leaf_temperature = environment.leaf_temperature;
        // External mode preserves the supplied leaf VPD as authoritative input;
        // it must not silently derive a replacement from the supplied temperature.
        state.leaf_vpd = environment.leaf_vpd;
        state.status = "external_temperature_used";
        state.source = "external_leaf_temperature";
        return;
    }

    if (!valid_profile(profile)) {
        state.status = "missing_leaf_energy_profile";
        return;
    }
    const double relative_low = environment.air_temperature.value - profile.air_relative_bracket_k;
    const double relative_high = environment.air_temperature.value + profile.air_relative_bracket_k;
    const double low = std::max({profile.solver_min_temperature_c, relative_low,
                                 kMinimumSupportedVaporPressureTemperatureC,
                                 non_condensing_lower_bound(environment.actual_vapor_pressure)});
    const double high = std::min({profile.solver_max_temperature_c, relative_high,
                                  kMaximumSupportedVaporPressureTemperatureC});
    if (!(finite(low) && finite(high) && low < high)) {
        state.status = "invalid_temperature_domain";
        return;
    }

    LeafEnergyEvaluation lower = evaluate_leaf_energy_residual(
        units::Celsius{low}, environment, lighting, profile, gas_exchange_profile,
        beta_hydraulic, characteristic_dimension_m);
    if (!lower.available) {
        state.status = lower.status;
        return;
    }
    LeafEnergyEvaluation upper = evaluate_leaf_energy_residual(
        units::Celsius{high}, environment, lighting, profile, gas_exchange_profile,
        beta_hydraulic, characteristic_dimension_m);
    if (!upper.available) {
        state.status = upper.status;
        return;
    }

    if (std::abs(lower.energy_residual_w_m2) <= profile.residual_tolerance_w_m2) {
        assign_final(state, lower, 0);
        return;
    }
    if (std::abs(upper.energy_residual_w_m2) <= profile.residual_tolerance_w_m2) {
        assign_final(state, upper, 0);
        return;
    }
    if (std::signbit(lower.energy_residual_w_m2) == std::signbit(upper.energy_residual_w_m2)) {
        state.status = "solver_no_bracket";
        return;
    }

    double bracket_low = low;
    double bracket_high = high;
    LeafEnergyEvaluation midpoint;
    for (std::uint32_t iteration = 1; iteration <= profile.max_iterations; ++iteration) {
        const double midpoint_temperature = bracket_low + ((bracket_high - bracket_low) * 0.5);
        midpoint = evaluate_leaf_energy_residual(
            units::Celsius{midpoint_temperature}, environment, lighting, profile,
            gas_exchange_profile, beta_hydraulic, characteristic_dimension_m);
        if (!midpoint.available) {
            state.status = midpoint.status;
            state.solver_iterations = iteration;
            return;
        }
        const double width = bracket_high - bracket_low;
        if (width <= profile.temperature_tolerance_k &&
            std::abs(midpoint.energy_residual_w_m2) <= profile.residual_tolerance_w_m2) {
            assign_final(state, midpoint, iteration);
            return;
        }
        if (std::signbit(lower.energy_residual_w_m2) != std::signbit(midpoint.energy_residual_w_m2)) {
            bracket_high = midpoint_temperature;
            upper = midpoint;
        } else {
            bracket_low = midpoint_temperature;
            lower = midpoint;
        }
    }
    state.status = "solver_iteration_limit";
    state.solver_iterations = profile.max_iterations;
}

} // namespace cannaville::leafenergy
