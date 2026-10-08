#pragma once

#include "Simulation/Core/Units.hpp"
#include <string>
#include <optional>
#include <vector>

namespace cannaville::gasexchange {

struct FvCBParameters {
    // Biochemical capacities at normalization temperature (usually 25 C)
    double vcmax25{0.0}; // umol m-2 s-1
    double jmax25{0.0};  // umol m-2 s-1
    double tpu25{0.0};   // umol m-2 s-1
    double rd25{0.0};    // umol m-2 s-1

    // Quantum efficiency / electron transport curvature
    double theta_j{0.7}; // empirical curvature parameter
    double alpha{0.24};  // quantum yield of electron transport

    // Temperature response parameters (e.g., Arrhenius activation energies, J/mol)
    double ea_vcmax{65330.0};
    double ea_jmax{43540.0};
    double ea_rd{46390.0};
    double ea_tpu{53100.0};
    double hd_jmax{200000.0}; // Deactivation energy J/mol
    double sv_jmax{650.0};    // Entropy J/K/mol
    double hd_vcmax{200000.0};
    double sv_vcmax{650.0};
    double hd_tpu{200000.0};
    double sv_tpu{650.0};

    // Michaelis-Menten constants at 25 C and their activation energies
    double kc25{404.9}; // umol mol-1
    double ea_kc{79430.0};
    double ko25{278400.0}; // umol mol-1 (278.4 mmol mol-1)
    double ea_ko{36380.0};

    // Photorespiratory compensation point at 25 C and activation energy
    double gamma_star25{42.75}; // umol mol-1
    double ea_gamma_star{37830.0};
    
    // Normalization temperature
    double t_ref_k{298.15};
};

struct MedlynParameters {
    double g0{0.01}; // residual conductance (mol m-2 s-1)
    double g1{3.0};  // slope parameter (kPa^0.5)
};

// Numerical domain guard for the Medlyn model to prevent division by zero or biologically 
// meaningless infinite stomatal conductance at exactly zero VPD.
constexpr double kMedlynMinimumVPDKPa = 0.05; // kPa, below which stomatal conductance is clamped/evaluated at this floor.

struct CalibrationProfile {
    std::string id;
    FvCBParameters fvcb;
    MedlynParameters medlyn;
    bool is_configured{false};
};

enum class ConvergenceStatus {
    NotRun,
    Converged,
    FailedToConverge,
    MissingLeafTemperature,
    MissingCalibrationProfile,
    NegativeVPD,
};

struct GasExchangeState {
    units::AssimilationMicromolesPerSquareMeterSecond net_assimilation{0.0};
    units::IntercellularCO2MicromolesPerMole intercellular_co2{0.0};
    units::StomatalConductanceMolesPerSquareMeterSecond stomatal_conductance{0.0};
    double g1_reference{0.0};
    double g1_effective{0.0};
    double beta_hydraulic{1.0};
    ConvergenceStatus status{ConvergenceStatus::NotRun};
    std::string profile_id;
};

// Generic/unconfigured profile that cannot silently simulate
CalibrationProfile get_unconfigured_profile();
CalibrationProfile get_synthetic_vegetative_test_profile();
CalibrationProfile get_synthetic_high_capacity_test_profile();
CalibrationProfile get_synthetic_flowering_test_profile();

// Fetch by ID
CalibrationProfile get_profile(const std::string& id);

// Coupled solver
// Assumptions for Medlyn:
// - CO2 ambient (ca) is assumed to be leaf surface CO2 (cs), i.e., boundary layer conductance is deferred (infinite).
// - VPD is explicitly leaf-to-air VPD. If only air VPD is available, this should either be estimated or an approximation state must be explicitly requested.
void solve_coupled_gas_exchange(
    GasExchangeState& state,
    const CalibrationProfile& profile,
    units::PPFDMicromolesPerSquareMeterSecond ppfd,
    units::CO2MicromolesPerMole ambient_co2,
    units::VPDKPa leaf_to_air_vpd,
    std::optional<units::Celsius> leaf_temperature,
    units::AtmosphericPressureKPa pressure,
    double beta_hydraulic = 1.0
);

} // namespace cannaville::gasexchange
