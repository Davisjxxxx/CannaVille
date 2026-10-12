#pragma once

#include "Simulation/Core/Units.hpp"
#include "Simulation/Environment/EnvironmentState.hpp"
#include "Simulation/GasExchange/GasExchange.hpp"
#include "Simulation/Lighting/LightingState.hpp"
#include "Simulation/Transpiration/Transpiration.hpp"

#include <optional>
#include <cstdint>
#include <string>

namespace cannaville::leafenergy {

// P1B.5 area contract: all energy fluxes and P1B.4.1 transpiration fluxes are
// expressed per projected one-sided leaf area. surface_factor applies only to
// two-sided radiation/heat exchange. It never multiplies transpiration or the
// effective plant leaf area used for root-zone withdrawal.
enum class AreaBasis {
    ProjectedOneSidedLeafArea,
};

std::string to_string(AreaBasis basis);
AreaBasis area_basis_from_string(const std::string& value);

enum class LeafTemperatureMode {
    ExternalLeafTemperature,
    SolvedLeafEnergyBalance,
    Unavailable,
};

std::string to_string(LeafTemperatureMode mode);
LeafTemperatureMode leaf_temperature_mode_from_string(const std::string& value);

struct LeafEnergyProfile {
    std::string profile_id;
    std::string model_version{"leaf_energy_balance.steady_state.v1"};
    LeafTemperatureMode temperature_mode{LeafTemperatureMode::ExternalLeafTemperature};
    double shortwave_absorptance{0.0};
    double longwave_emissivity{0.0};
    double surface_factor{1.0};
    AreaBasis area_basis{AreaBasis::ProjectedOneSidedLeafArea};
    double solver_min_temperature_c{-40.0};
    double solver_max_temperature_c{100.0};
    double air_relative_bracket_k{80.0};
    double temperature_tolerance_k{0.001};
    double residual_tolerance_w_m2{0.01};
    std::uint32_t max_iterations{32};
    std::string source;
    std::string evidence_classification;
    std::string calibration_status;
    std::string valid_domain;
    bool is_configured{false};
};

LeafEnergyProfile get_unconfigured_profile();
LeafEnergyProfile get_synthetic_test_profile();

struct LeafEnergyEvaluation {
    bool available{false};
    std::string status{"not_run"};
    units::Celsius trial_leaf_temperature{};
    std::optional<units::VPDKPa> leaf_vpd;
    double absorbed_shortwave_w_m2{0.0};
    double net_longwave_w_m2{0.0};
    double sensible_heat_flux_w_m2{0.0};
    double latent_heat_flux_w_m2{0.0};
    double energy_residual_w_m2{0.0};
    std::uint32_t iterations{0};
    gasexchange::GasExchangeState gas_exchange;
    transpiration::BoundaryLayerConductance boundary_layer;
    transpiration::TranspirationResult transpiration;
};

struct LeafEnergyState {
    std::optional<units::Celsius> leaf_temperature;
    std::optional<units::VPDKPa> leaf_vpd;
    std::optional<double> absorbed_shortwave_w_m2;
    std::optional<double> net_longwave_w_m2;
    std::optional<double> sensible_heat_flux_w_m2;
    std::optional<double> latent_heat_flux_w_m2;
    std::optional<double> energy_residual_w_m2;
    std::uint32_t solver_iterations{0};
    std::string status{"not_run"};
    std::string source;
    std::string model_version;
};

LeafEnergyEvaluation evaluate_leaf_energy_residual(
    units::Celsius trial_leaf_temperature,
    const environment::EnvironmentState& environment,
    const lighting::LightingState& lighting,
    const LeafEnergyProfile& profile,
    const gasexchange::CalibrationProfile& gas_exchange_profile,
    std::optional<double> beta_hydraulic,
    std::optional<double> characteristic_dimension_m
);

void solve_leaf_energy_balance(
    LeafEnergyState& state,
    const LeafEnergyProfile& profile,
    const environment::EnvironmentState& environment,
    const lighting::LightingState& lighting,
    const gasexchange::CalibrationProfile& gas_exchange_profile,
    std::optional<double> beta_hydraulic,
    std::optional<double> characteristic_dimension_m
);

double latent_heat_of_vaporization_molar_j_mol(units::Celsius reference_temperature);

} // namespace cannaville::leafenergy
