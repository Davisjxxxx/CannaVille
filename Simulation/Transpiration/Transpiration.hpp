#pragma once

#include "Simulation/Core/Units.hpp"
#include <optional>
#include <string>

namespace cannaville::transpiration {

struct AirProperties {
    double kinematic_viscosity{1.51e-5}; // m^2 s^-1
    double vapor_diffusivity{2.42e-5};   // m^2 s^-1
    double molar_density{41.6};          // mol m^-3
};

// Calculate air properties dynamically based on temperature and pressure
AirProperties calculate_air_properties(
    units::Celsius air_temperature,
    units::AtmosphericPressureKPa pressure
);

enum class ScientificDomainStatus {
    Valid,
    UnsupportedFlowRegime,
    InvalidDimension
};

struct BoundaryLayerConductance {
    std::optional<double> value_mol_m2_s{std::nullopt};
    ScientificDomainStatus status{ScientificDomainStatus::Valid};
    std::string regime{"uninitialized"};
};

BoundaryLayerConductance calculate_boundary_layer_conductance(
    units::AirflowMetersPerSecond air_velocity,
    units::Meters characteristic_dimension,
    const AirProperties& air_props,
    units::Celsius leaf_temperature,
    units::Celsius air_temperature
);

struct TranspirationResult {
    double flux_mol_m2_s{0.0};
    double total_conductance_mol_m2_s{0.0};
    double leaf_air_vapor_gradient_mol_mol{0.0};
    std::string status{"Valid"};
};

// Calculate leaf transpiration flux
// Assumptions:
// - Series resistance: r_total = r_stomatal + r_boundary => g_total = (g_s * g_b) / (g_s + g_b)
// - One-sided (hypostomatous) or amphistomatous handled externally, here we compute per unit transpiring area.
TranspirationResult calculate_transpiration(
    units::StomatalConductanceMolesPerSquareMeterSecond stomatal_conductance,
    const BoundaryLayerConductance& boundary_layer,
    units::VPDKPa leaf_to_air_vpd,
    units::AtmosphericPressureKPa pressure
);

} // namespace cannaville::transpiration
