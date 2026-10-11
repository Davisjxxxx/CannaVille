#pragma once
#include "Simulation/Core/Units.hpp"
#include <string>
#include <optional>
#include <vector>

namespace cannaville::rootzone {

enum class CurveBasis {
    Desorption,
    Sorption,
    UnknownCombined
};

enum class EvidenceClassification {
    PhysicalConstant,
    SubstrateProfile,
    ModelParameter,
    CannabisReference,
    CultivarProfile,
    SyntheticTestValue,
    RequiresCalibration,
    ReferenceOnly
};

struct SubstrateHydraulicProfile {
    std::string profile_id;
    double theta_r_m3_m3;
    double theta_s_m3_m3;
    double alpha_per_meter; 
    double n;
    double m; 
    CurveBasis curve_basis;
    std::string source;
    EvidenceClassification evidence_classification;
    std::string valid_vwc_domain;
    std::string limitations;
    
    bool is_valid() const { return !profile_id.empty() && profile_id != "missing"; }
};

SubstrateHydraulicProfile get_missing_substrate_profile();
SubstrateHydraulicProfile get_synthetic_substrate_test_profile_a();
SubstrateHydraulicProfile get_synthetic_substrate_test_profile_b();

enum class BetaModelForm {
    LinearPiecewise
};

struct HydraulicStressTransferProfile {
    std::string profile_id;
    BetaModelForm model_form;
    std::vector<std::pair<double, double>> calibration_points; // <psi_MPa, beta>
    std::string cannabis_applicability;
    std::string source;
    EvidenceClassification evidence_classification;
    std::string valid_domain;
    std::string limitations;

    bool is_valid() const { return !profile_id.empty() && profile_id != "missing"; }
};

HydraulicStressTransferProfile get_missing_stress_profile();
HydraulicStressTransferProfile get_synthetic_stress_test_profile();

enum class HydraulicStatus {
    Valid,
    MissingSubstrateHydraulicProfile,
    MissingHydraulicStressCalibration,
    OutsideRetentionModelDomain,
    OutsideStressCalibrationDomain,
    HydraulicLimitationDisabled,
    InsufficientRootzoneWater,
    HydraulicStateUnavailable,
    UnrestrictedWaterAccess,
    NumericalFailure
};


inline std::string to_string(HydraulicStatus status) {
    switch (status) {
        case HydraulicStatus::Valid: return "Valid";
        case HydraulicStatus::MissingSubstrateHydraulicProfile: return "MissingSubstrateHydraulicProfile";
        case HydraulicStatus::MissingHydraulicStressCalibration: return "MissingHydraulicStressCalibration";
        case HydraulicStatus::OutsideRetentionModelDomain: return "OutsideRetentionModelDomain";
        case HydraulicStatus::OutsideStressCalibrationDomain: return "OutsideStressCalibrationDomain";
        case HydraulicStatus::HydraulicLimitationDisabled: return "HydraulicLimitationDisabled";
        case HydraulicStatus::InsufficientRootzoneWater: return "InsufficientRootzoneWater";
        case HydraulicStatus::HydraulicStateUnavailable: return "HydraulicStateUnavailable";
        case HydraulicStatus::UnrestrictedWaterAccess: return "UnrestrictedWaterAccess";
        case HydraulicStatus::NumericalFailure: return "NumericalFailure";
        default: return "Unknown";
    }
}

struct HydraulicState {
    std::string substrate_profile_id;
    std::string stress_transfer_profile_id;
    double effective_saturation{1.0};
    double matric_pressure_head_meters{0.0};
    double matric_potential_mpa{0.0};
    std::optional<double> beta_hydraulic;
    HydraulicStatus status{HydraulicStatus::Valid};
};

struct EffectiveMedlyn {
    double g1_reference{0.0};
    double g1_effective{0.0};
    std::optional<double> beta_hydraulic;
};

void evaluate_van_genuchten(
    double theta_vwc,
    const SubstrateHydraulicProfile& profile,
    HydraulicState& state_out
);

void evaluate_beta_hydraulic(
    double matric_potential_mpa,
    const HydraulicStressTransferProfile& profile,
    HydraulicState& state_out
);

HydraulicState compute_substrate_hydraulic_limitation(
    double theta_vwc,
    const SubstrateHydraulicProfile& substrate_profile,
    const HydraulicStressTransferProfile& stress_profile
);

HydraulicState compute_dwc_hydraulic_limitation(
    bool has_sufficient_water,
    bool requires_geometry_for_access,
    bool explicit_unrestricted_override
);

EffectiveMedlyn apply_hydraulic_limitation_to_medlyn(
    double g1_reference,
    std::optional<double> beta_hydraulic
);

} // namespace cannaville::rootzone
