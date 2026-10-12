#include "Simulation/Transpiration/Transpiration.hpp"
#include <cmath>
#include <algorithm>

namespace cannaville::transpiration {

constexpr double kUniversalGasConstant = 8.31446261815324; // J/(mol K)
constexpr double kStandardPressureKPa = 101.325;
constexpr double kGravity = 9.81; // m/s^2
constexpr double kSpecificGasConstantDryAir = 287.058; // J kg^-1 K^-1
constexpr double kAirHeatCapacity = 1005.0; // J kg^-1 K^-1, engineering air property
constexpr double kPrandtlNumberAir = 0.71; // dimensionless, engineering air property

AirProperties calculate_air_properties(
    units::Celsius air_temperature,
    units::AtmosphericPressureKPa pressure
) {
    AirProperties props;
    double tk = air_temperature.value + 273.15;
    
    // Molar density (Ideal Gas Law: n/V = P / (R T)), P in Pa
    props.molar_density = (pressure.value * 1000.0) / (kUniversalGasConstant * tk);
    props.air_density_kg_m3 = (pressure.value * 1000.0) / (kSpecificGasConstantDryAir * tk);
    props.cp_air_j_kg_k = kAirHeatCapacity;
    props.prandtl_number = kPrandtlNumberAir;
    
    // Campbell & Norman (1998) Eq 3.9 and A.3
    // Vapor diffusivity in air
    double temp_ratio = tk / 273.15;
    double temp_ratio_1_75 = std::pow(temp_ratio, 1.75);
    double pressure_ratio = kStandardPressureKPa / pressure.value;
    
    props.vapor_diffusivity = 2.12e-5 * pressure_ratio * temp_ratio_1_75;
    props.kinematic_viscosity = 1.33e-5 * pressure_ratio * temp_ratio_1_75;
    props.thermal_diffusivity = props.kinematic_viscosity / props.prandtl_number;
    
    return props;
}

BoundaryLayerConductance calculate_boundary_layer_conductance(
    units::AirflowMetersPerSecond air_velocity,
    units::Meters characteristic_dimension,
    const AirProperties& air_props,
    units::Celsius leaf_temperature,
    units::Celsius air_temperature
) {
    BoundaryLayerConductance result;
    
    if (characteristic_dimension.value <= 0.0) {
        result.status = ScientificDomainStatus::InvalidDimension;
        result.regime = "unsupported";
        return result;
    }
    
    double u = std::max(0.0, air_velocity.value);
    double d = characteristic_dimension.value;
    double nu = air_props.kinematic_viscosity;
    double dv = air_props.vapor_diffusivity;
    
    // Forced convection
    double Re = (u * d) / nu;
    double Sc = nu / dv;
    
    double Sh_forced = 0.0;
    double Nu_forced = 0.0;
    if (Re > 0.0) {
        // Laminar flat plate forced convection (Campbell & Norman 1998, Eq 7.28)
        Sh_forced = 0.66 * std::sqrt(Re) * std::cbrt(Sc);
        const double Pr = air_props.prandtl_number;
        Nu_forced = 0.66 * std::sqrt(Re) * std::cbrt(Pr);
    }
    
    // Free convection (Campbell & Norman 1998, Eq 7.33)
    double tl_k = leaf_temperature.value + 273.15;
    double ta_k = air_temperature.value + 273.15;
    double delta_t = std::abs(tl_k - ta_k);
    
    double Gr = (kGravity * std::pow(d, 3) * delta_t) / (ta_k * nu * nu);
    
    double Sh_free = 0.0;
    double Nu_free = 0.0;
    if (Gr > 0.0) {
        Sh_free = 0.54 * std::pow(Gr * Sc, 0.25);
        Nu_free = 0.54 * std::pow(Gr * air_props.prandtl_number, 0.25);
    }
    
    // Combined convection (mixed regime)
    // DOMINANT_MODE_APPROXIMATION:
    // We take the maximum to ensure physically plausible conductance at zero wind speed.
    double Sh_mixed = std::max(Sh_forced, Sh_free);
    const double Nu_mixed = std::max(Nu_forced, Nu_free);
    
    if (Sh_mixed == Sh_free && u < 0.1 && delta_t > 0.0) {
        result.regime = "free_convection_dominant";
    } else if (Sh_mixed == Sh_forced) {
        result.regime = "forced_convection_dominant";
    } else {
        result.regime = "mixed_convection";
    }
    
    if (Re > 20000.0) {
        result.status = ScientificDomainStatus::UnsupportedFlowRegime;
        result.regime = "turbulent_unsupported";
        return result; // Explicitly do not return a valid laminar conductance
    }
    
    double g_bw_ms = (dv / d) * Sh_mixed;
    result.value_mol_m2_s = g_bw_ms * air_props.molar_density;
    result.value_heat_m_s = (air_props.thermal_diffusivity / d) * Nu_mixed;
    
    return result;
}

TranspirationResult calculate_transpiration(
    units::StomatalConductanceMolesPerSquareMeterSecond stomatal_conductance,
    const BoundaryLayerConductance& boundary_layer,
    units::VPDKPa leaf_to_air_vpd,
    units::AtmosphericPressureKPa pressure
) {
    TranspirationResult result;
    
    if (pressure.value <= 0.0) {
        result.status = "invalid_pressure";
        return result;
    }

    if (!boundary_layer.value_mol_m2_s.has_value()) {
        result.status = "unsupported_boundary_layer";
        return result;
    }
    
    double gs = stomatal_conductance.value;
    double gb = boundary_layer.value_mol_m2_s.value();
    
    // Series resistance for a single transpiring surface (e.g. hypostomatous leaf)
    // If amphistomatous, this represents total effective per-unit-area conductance.
    if (gs <= 0.0 || gb <= 0.0) {
        result.total_conductance_mol_m2_s = 0.0;
    } else {
        result.total_conductance_mol_m2_s = (gs * gb) / (gs + gb);
    }
    
    // Vapor gradient in mol/mol (mole fraction difference)
    // VPD is e_s(T_leaf) - e_a. 
    // Mole fraction difference = VPD / P_atm
    double delta_w = leaf_to_air_vpd.value / pressure.value;
    result.leaf_air_vapor_gradient_mol_mol = delta_w;
    
    // Flux E = g_total * delta_w
    result.flux_mol_m2_s = result.total_conductance_mol_m2_s * delta_w;
    
    if (delta_w < 0.0) {
        result.status = "condensation";
    }

    // The flux, including a genuine zero, is authoritative only after all
    // required conductance and pressure inputs have been validated.
    result.available = true;
    
    return result;
}

} // namespace cannaville::transpiration
