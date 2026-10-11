#include "Simulation/GasExchange/GasExchange.hpp"

#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace cannaville::gasexchange {

constexpr double kR = 8.31446261815324; // Universal gas constant J/(mol K)

CalibrationProfile get_unconfigured_profile() {
    CalibrationProfile p;
    p.id = "unconfigured";
    p.is_configured = false;
    return p;
}

CalibrationProfile get_synthetic_vegetative_test_profile() {
    CalibrationProfile p;
    p.id = "synthetic_vegetative_test";
    p.is_configured = true;
    p.fvcb.vcmax25 = 100.0;
    p.fvcb.jmax25 = 150.0;
    p.fvcb.tpu25 = 10.0;
    p.fvcb.rd25 = 1.0;
    p.medlyn.g1 = 3.5;
    return p;
}

CalibrationProfile get_synthetic_high_capacity_test_profile() {
    CalibrationProfile p;
    p.id = "synthetic_high_capacity_test";
    p.is_configured = true;
    p.fvcb.vcmax25 = 110.0;
    p.fvcb.jmax25 = 165.0;
    p.fvcb.tpu25 = 12.0;
    p.fvcb.rd25 = 1.5;
    p.medlyn.g1 = 4.0;
    return p;
}

CalibrationProfile get_synthetic_flowering_test_profile() {
    CalibrationProfile p;
    p.id = "synthetic_flowering_test";
    p.is_configured = true;
    p.fvcb.vcmax25 = 60.0;
    p.fvcb.jmax25 = 90.0;
    p.fvcb.tpu25 = 8.0;
    p.fvcb.rd25 = 0.8;
    p.medlyn.g1 = 3.0;
    return p;
}

CalibrationProfile get_profile(const std::string& id) {
    if (id == "synthetic_vegetative_test") return get_synthetic_vegetative_test_profile();
    if (id == "synthetic_high_capacity_test") return get_synthetic_high_capacity_test_profile();
    if (id == "synthetic_flowering_test") return get_synthetic_flowering_test_profile();
    return get_unconfigured_profile();
}

static double arrhenius(double param25, double ea, double tk, double tref) {
    return param25 * std::exp((ea * (tk - tref)) / (tref * kR * tk));
}

static double peaked_arrhenius(double param25, double ea, double hd, double sv, double tk, double tref) {
    double base = arrhenius(param25, ea, tk, tref);
    double num = 1.0 + std::exp((sv * tref - hd) / (kR * tref));
    double den = 1.0 + std::exp((sv * tk - hd) / (kR * tk));
    return base * (num / den);
}

void solve_coupled_gas_exchange(
    GasExchangeState& state,
    const CalibrationProfile& profile,
    units::PPFDMicromolesPerSquareMeterSecond ppfd,
    units::CO2MicromolesPerMole ambient_co2,
    units::VPDKPa leaf_to_air_vpd,
    std::optional<units::Celsius> leaf_temperature,
    units::AtmosphericPressureKPa pressure,
    std::optional<double> beta_hydraulic
) {
    state.profile_id = profile.id;
    if (beta_hydraulic.has_value()) {
        state.beta_hydraulic = std::clamp(beta_hydraulic.value(), 0.0, 1.0);
    } else {
        state.beta_hydraulic = std::nullopt;
    }
    state.g1_reference = profile.medlyn.g1;
    state.g1_effective = state.g1_reference * state.beta_hydraulic.value_or(1.0);

    if (!profile.is_configured) {
        state.status = ConvergenceStatus::MissingCalibrationProfile;
        return;
    }
    if (!leaf_temperature.has_value()) {
        state.status = ConvergenceStatus::MissingLeafTemperature;
        return;
    }
    if (leaf_to_air_vpd.value < 0.0) {
        state.status = ConvergenceStatus::NegativeVPD;
        return;
    }

    double tk = leaf_temperature->value + 273.15;
    double tref = profile.fvcb.t_ref_k;
    
    // Calculate temperature-adjusted FvCB parameters
    double vcmax = peaked_arrhenius(profile.fvcb.vcmax25, profile.fvcb.ea_vcmax, profile.fvcb.hd_vcmax, profile.fvcb.sv_vcmax, tk, tref);
    double jmax = peaked_arrhenius(profile.fvcb.jmax25, profile.fvcb.ea_jmax, profile.fvcb.hd_jmax, profile.fvcb.sv_jmax, tk, tref);
    double tpu = peaked_arrhenius(profile.fvcb.tpu25, profile.fvcb.ea_tpu, profile.fvcb.hd_tpu, profile.fvcb.sv_tpu, tk, tref);
    double rd = arrhenius(profile.fvcb.rd25, profile.fvcb.ea_rd, tk, tref);
    
    double kc = arrhenius(profile.fvcb.kc25, profile.fvcb.ea_kc, tk, tref);
    double ko = arrhenius(profile.fvcb.ko25, profile.fvcb.ea_ko, tk, tref);
    double gamma_star = arrhenius(profile.fvcb.gamma_star25, profile.fvcb.ea_gamma_star, tk, tref);
    
    // Oxygen concentration in umol/mol (approx 210,000 umol/mol)
    double o2 = 210000.0;
    
    // Calculate J from PPFD
    // theta_j * J^2 - (alpha * PPFD + Jmax) * J + alpha * PPFD * Jmax = 0
    double I2 = profile.fvcb.alpha * ppfd.value;
    double theta = profile.fvcb.theta_j;
    double J = 0.0;
    if (ppfd.value > 0.0) {
        double b = I2 + jmax;
        double c = I2 * jmax;
        double inner = b * b - 4.0 * theta * c;
        if (inner < 0.0) inner = 0.0;
        J = (b - std::sqrt(inner)) / (2.0 * theta);
    }

    // Iterative solver for Ci
    // Solved variables: ci, An, gsw
    // Maximum iterations: 100
    // Convergence criterion: difference in ci < 1e-4
    double ca = ambient_co2.value; // Ambient CO2 assumed to be leaf surface CO2 (infinite boundary layer)
    double ci = ca * 0.7; // Initial guess
    
    double D = std::max(kMedlynMinimumVPDKPa, leaf_to_air_vpd.value); // Prevent division by zero and unrealistic infinite conductance at near-zero VPD
    double g0 = profile.medlyn.g0;
    double g1 = state.g1_effective;
    
    double An = 0.0;
    double gsw = 0.0;
    
    bool converged = false;
    for (int iter = 0; iter < 100; ++iter) {
        double Ac = vcmax * (ci - gamma_star) / (ci + kc * (1.0 + o2 / ko));
        double Aj = (J / 4.0) * (ci - gamma_star) / (ci + 2.0 * gamma_star);
        double Ap = 3.0 * tpu; // TPU limit
        
        // Net assimilation
        double A = std::min({Ac, Aj, Ap}) - rd;
        An = A;
        
        // Medlyn model for stomatal conductance to water vapor (mol m-2 s-1)
        // If An < 0 (respiration exceeds gross photosynthesis), stomata tend to close to g0.
        double A_for_medlyn = std::max(0.0, An);
        gsw = g0 + 1.6 * (1.0 + g1 / std::sqrt(D)) * A_for_medlyn / ca;
        
        // Stomatal conductance to CO2
        double gsc = gsw / 1.6;
        
        // Calculate new ci
        double new_ci = ca - An / gsc;
        
        // Clamp ci to sensible bounds
        if (new_ci < 0.0) new_ci = 0.0;
        if (new_ci > ca) new_ci = ca;
        
        if (std::abs(new_ci - ci) < 1e-4) {
            ci = new_ci;
            converged = true;
            break;
        }
        ci = 0.5 * ci + 0.5 * new_ci; // Damping for stability
    }
    
    if (converged) {
        state.status = ConvergenceStatus::Converged;
        state.net_assimilation.value = An;
        state.intercellular_co2.value = ci;
        state.stomatal_conductance.value = gsw;
    } else {
        state.status = ConvergenceStatus::FailedToConverge;
        // No silent fallback to arbitrary values on non-convergence.
    }
}

} // namespace cannaville::gasexchange
