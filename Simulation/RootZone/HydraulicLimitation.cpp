#include "Simulation/RootZone/HydraulicLimitation.hpp"
#include <cmath>
#include <algorithm>

namespace cannaville::rootzone {

constexpr double kMetersHeadToMPa = 0.00980665;

SubstrateHydraulicProfile get_missing_substrate_profile() {
    return {"missing", 0, 0, 0, 0, 0, CurveBasis::UnknownCombined, "", EvidenceClassification::RequiresCalibration, "", ""};
}

SubstrateHydraulicProfile get_synthetic_substrate_test_profile_a() {
    // Synthetic test values, not for production
    return {
        "synthetic_test_a",
        0.05, 0.60, 2.0, 1.5, 1.0 - (1.0 / 1.5),
        CurveBasis::Desorption,
        "synthetic_fixture",
        EvidenceClassification::SyntheticTestValue,
        "0.05 - 0.60",
        "Not biologically validated."
    };
}

SubstrateHydraulicProfile get_synthetic_substrate_test_profile_b() {
    return {
        "synthetic_test_b",
        0.10, 0.50, 1.5, 2.0, 1.0 - (1.0 / 2.0),
        CurveBasis::Desorption,
        "synthetic_fixture",
        EvidenceClassification::SyntheticTestValue,
        "0.10 - 0.50",
        "Not biologically validated."
    };
}

HydraulicStressTransferProfile get_missing_stress_profile() {
    return {"missing", BetaModelForm::LinearPiecewise, {}, "", "", EvidenceClassification::RequiresCalibration, "", ""};
}

HydraulicStressTransferProfile get_synthetic_stress_test_profile() {
    return {
        "synthetic_stress_test",
        BetaModelForm::LinearPiecewise,
        {{0.0, 1.0}, {-1.0, 1.0}, {-2.0, 0.0}, {-3.0, 0.0}},
        "none",
        "synthetic_fixture",
        EvidenceClassification::SyntheticTestValue,
        "0 to -3 MPa",
        "Not biologically validated."
    };
}

void evaluate_van_genuchten(
    double theta_vwc,
    const SubstrateHydraulicProfile& profile,
    HydraulicState& state_out
) {
    if (!profile.is_valid()) {
        state_out.status = HydraulicStatus::MissingSubstrateHydraulicProfile;
        return;
    }
    
    state_out.substrate_profile_id = profile.profile_id;
    
    if (profile.theta_s_m3_m3 <= profile.theta_r_m3_m3 || profile.n <= 1.0 || profile.alpha_per_meter <= 0.0) {
        state_out.status = HydraulicStatus::NumericalFailure;
        return;
    }
    
    if (theta_vwc > profile.theta_s_m3_m3) {
        state_out.status = HydraulicStatus::OutsideRetentionModelDomain;
        // Do not silently clamp. But we can evaluate at theta_s for continuity if needed.
        // The prompt says: "Return domain/status information instead of fake finite values where the formulation approaches a singular dry limit."
        return;
    }
    
    if (theta_vwc <= profile.theta_r_m3_m3) {
        state_out.status = HydraulicStatus::OutsideRetentionModelDomain;
        return;
    }
    
    double se = (theta_vwc - profile.theta_r_m3_m3) / (profile.theta_s_m3_m3 - profile.theta_r_m3_m3);
    state_out.effective_saturation = se;
    
    // h = [Se^(-1/m) - 1]^(1/n) / alpha
    double se_inv_m = std::pow(se, -1.0 / profile.m);
    if (se_inv_m < 1.0) se_inv_m = 1.0; // numerical safety near saturation
    
    double h = std::pow(se_inv_m - 1.0, 1.0 / profile.n) / profile.alpha_per_meter;
    
    state_out.matric_pressure_head_meters = h; // this is suction head, typically positive
    state_out.matric_potential_mpa = -h * kMetersHeadToMPa;
    state_out.status = HydraulicStatus::Valid;
}

void evaluate_beta_hydraulic(
    double matric_potential_mpa,
    const HydraulicStressTransferProfile& profile,
    HydraulicState& state_out
) {
    if (!profile.is_valid()) {
        state_out.status = HydraulicStatus::MissingHydraulicStressCalibration;
        return;
    }
    
    state_out.stress_transfer_profile_id = profile.profile_id;
    
    if (profile.model_form == BetaModelForm::LinearPiecewise) {
        if (profile.calibration_points.empty()) {
            state_out.status = HydraulicStatus::NumericalFailure;
            return;
        }
        
        // Find interval
        double psi = matric_potential_mpa;
        const auto& pts = profile.calibration_points;
        
        if (psi > pts.front().first) { // assuming sorted descending e.g. 0, -1, -2
            state_out.status = HydraulicStatus::OutsideStressCalibrationDomain;
            return;
        }
        if (psi < pts.back().first) {
            state_out.status = HydraulicStatus::OutsideStressCalibrationDomain;
            return;
        }
        
        for (size_t i = 0; i < pts.size() - 1; ++i) {
            double p0 = pts[i].first;
            double b0 = pts[i].second;
            double p1 = pts[i+1].first;
            double b1 = pts[i+1].second;
            
            // Allow descending order: 0, -1, -2
            if ((p0 >= psi && psi >= p1) || (p0 <= psi && psi <= p1)) {
                double t = (psi - p0) / (p1 - p0);
                double beta = b0 + t * (b1 - b0);
                state_out.beta_hydraulic = std::clamp(beta, 0.0, 1.0);
                state_out.status = HydraulicStatus::Valid;
                return;
            }
        }
    } else {
        state_out.status = HydraulicStatus::NumericalFailure;
    }
}

HydraulicState compute_substrate_hydraulic_limitation(
    double theta_vwc,
    const SubstrateHydraulicProfile& substrate_profile,
    const HydraulicStressTransferProfile& stress_profile
) {
    HydraulicState state;
    evaluate_van_genuchten(theta_vwc, substrate_profile, state);
    if (state.status != HydraulicStatus::Valid) return state;
    
    evaluate_beta_hydraulic(state.matric_potential_mpa, stress_profile, state);
    return state;
}

HydraulicState compute_dwc_hydraulic_limitation(
    bool has_sufficient_water,
    bool requires_geometry_for_access,
    bool explicit_unrestricted_override
) {
    HydraulicState state;
    state.substrate_profile_id = "DWC";
    state.stress_transfer_profile_id = "DWC";
    
    if (requires_geometry_for_access) {
        state.status = HydraulicStatus::HydraulicStateUnavailable;
        state.beta_hydraulic = 1.0; 
    } else if (!has_sufficient_water) {
        state.status = HydraulicStatus::InsufficientRootzoneWater;
        state.beta_hydraulic = 0.0; // short explicitly
    } else if (explicit_unrestricted_override) {
        state.status = HydraulicStatus::UnrestrictedWaterAccess;
        state.beta_hydraulic = 1.0;
    } else {
        state.status = HydraulicStatus::HydraulicStateUnavailable;
        state.beta_hydraulic = 1.0;
    }
    return state;
}

EffectiveMedlyn apply_hydraulic_limitation_to_medlyn(
    double g1_reference,
    double beta_hydraulic
) {
    EffectiveMedlyn result;
    result.g1_reference = g1_reference;
    // g1_effective = g1_reference * beta_hydraulic
    result.beta_hydraulic = std::clamp(beta_hydraulic, 0.0, 1.0);
    result.g1_effective = result.g1_reference * result.beta_hydraulic;
    return result;
}

} // namespace cannaville::rootzone
