#pragma once

#include "Simulation/Core/Units.hpp"

#include <optional>
#include <string>
#include <vector>

namespace cannaville::consequences {

enum class DevelopmentStage {
    Seedling,
    Vegetative,
    Flowering,
    HarvestReady,
};

std::string to_string(DevelopmentStage stage);
DevelopmentStage development_stage_from_string(const std::string& value);

struct StageProfile {
    DevelopmentStage stage{DevelopmentStage::Seedling};
    double required_opportunity_s{0.0};
    double assimilation_reference_umol_m2_s{0.0};
    std::optional<double> minimum_elapsed_stage_s;
    std::optional<double> minimum_dark_duration_s;
};

struct ExposureProfile {
    double onset{0.0};
    double full{1.0};
    double critical{1.0};
    double recovery_rate_per_s{0.0};
    double damage_onset{1.0};
    double damage_scale_s{1.0};
};

struct PresentationThresholds {
    double stress_onset{0.2};
    double recovery_onset{0.2};
    double damage_onset{0.2};
    double critical_onset{0.8};
    double thriving_opportunity{0.8};
};

struct PlantConsequenceProfile {
    std::string profile_id;
    std::string model_version{"p2_growth_consequences.v1"};
    std::vector<StageProfile> stages;
    ExposureProfile water;
    ExposureProfile thermal;
    ExposureProfile radiation;
    PresentationThresholds presentation;
    std::string source;
    std::string evidence_classification;
    std::string calibration_status;
    std::string valid_domain;
    bool is_configured{false};
};

PlantConsequenceProfile get_unconfigured_profile();
PlantConsequenceProfile get_synthetic_test_profile();
std::vector<std::string> validate_profile(const PlantConsequenceProfile& profile);

struct PlantConsequenceState {
    std::string model_version;
    DevelopmentStage stage{DevelopmentStage::Seedling};
    double stage_progress_0_1{0.0};
    double stage_opportunity_accumulated_s{0.0};
    double stage_elapsed_s{0.0};

    double potential_opportunity_s{0.0};
    double achieved_opportunity_s{0.0};
    double lost_opportunity_s{0.0};
    double current_opportunity_signal_0_1{0.0};
    bool opportunity_available{false};

    double water_current_severity_0_1{0.0};
    double thermal_current_severity_0_1{0.0};
    double radiation_current_severity_0_1{0.0};
    bool water_available{false};
    bool thermal_available{false};
    bool radiation_available{false};

    double water_exposure_dose_s{0.0};
    double thermal_exposure_dose_s{0.0};
    double radiation_exposure_dose_s{0.0};

    double water_damage_dose_s{0.0};
    double thermal_damage_dose_s{0.0};
    double radiation_damage_dose_s{0.0};

    double water_recovery_0_1{0.0};
    double thermal_recovery_0_1{0.0};
    double radiation_recovery_0_1{0.0};

    double water_damage_0_1{0.0};
    double thermal_damage_0_1{0.0};
    double radiation_damage_0_1{0.0};

    std::string integration_status{"not_run"};
    double last_integrated_simulation_time_s{0.0};
};

struct SupportingVariable {
    std::string name;
    std::string unit;
    double value{0.0};
};

struct ReasonRecord {
    std::string reason_code;
    double severity_0_1{0.0};
    double start_time_s{0.0};
    double duration_s{0.0};
    std::string source_channel;
    std::string status;
    std::vector<SupportingVariable> supporting_variables;
};

struct PlantConsequencePresentation {
    DevelopmentStage growth_stage{DevelopmentStage::Seedling};
    double stage_progress_0_1{0.0};
    double size_proxy_0_1{0.0};
    double vigor_signal_0_1{0.0};
    double wilt_severity_0_1{0.0};
    double growth_slowdown_0_1{0.0};
    double heat_stress_severity_0_1{0.0};
    double radiation_stress_severity_0_1{0.0};
    double damage_severity_0_1{0.0};
    double recovery_severity_0_1{0.0};
    std::string derived_condition{"UNAVAILABLE"};
    std::vector<ReasonRecord> reason_codes;
};

struct SegmentInputs {
    double start_time_s{0.0};
    double duration_s{0.0};
    std::optional<double> net_assimilation_umol_m2_s;
    std::optional<double> hydraulic_beta;
    bool water_allocation_available{false};
    double requested_water_mol{0.0};
    double realized_water_mol{0.0};
    std::optional<double> leaf_temperature_c;
    std::optional<double> absorbed_shortwave_w_m2;
    double accumulated_dark_duration_s{0.0};
    std::string gas_exchange_status;
    std::string transpiration_status;
};

void integrate_segment(
    PlantConsequenceState& state,
    PlantConsequencePresentation& presentation,
    const PlantConsequenceProfile& profile,
    const SegmentInputs& inputs
);

} // namespace cannaville::consequences
