#include "Simulation/Consequences/ConsequenceState.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace cannaville::consequences {

namespace {

constexpr double kEpsilon = 1e-12;

double clamp01(double value) {
    return std::clamp(value, 0.0, 1.0);
}

bool finite_nonnegative(double value) {
    return std::isfinite(value) && value >= 0.0;
}

const StageProfile* stage_profile_for(const PlantConsequenceProfile& profile, DevelopmentStage stage) {
    for (const StageProfile& candidate : profile.stages) {
        if (candidate.stage == stage) return &candidate;
    }
    return nullptr;
}

DevelopmentStage next_stage(DevelopmentStage stage) {
    switch (stage) {
        case DevelopmentStage::Seedling: return DevelopmentStage::Vegetative;
        case DevelopmentStage::Vegetative: return DevelopmentStage::Flowering;
        case DevelopmentStage::Flowering: return DevelopmentStage::HarvestReady;
        case DevelopmentStage::HarvestReady: return DevelopmentStage::HarvestReady;
    }
    return DevelopmentStage::HarvestReady;
}

double map_exposure(double value, const ExposureProfile& profile) {
    if (!std::isfinite(value) || !std::isfinite(profile.onset) || !std::isfinite(profile.full) ||
        profile.full <= profile.onset) return std::numeric_limits<double>::quiet_NaN();
    if (value <= profile.onset) return 0.0;
    if (value >= profile.full) return 1.0;
    return clamp01((value - profile.onset) / (profile.full - profile.onset));
}

void update_channel(
    double severity,
    const ExposureProfile& profile,
    double dt,
    double& current,
    bool& available,
    double& dose,
    double& damage_dose,
    double& recovery,
    double& damage
) {
    current = clamp01(severity);
    available = true;
    dose += current * dt;
    damage_dose += std::max(0.0, current - profile.damage_onset) * dt;
    const double recovery_factor = std::exp(-profile.recovery_rate_per_s * dt);
    recovery = clamp01(recovery * recovery_factor + current * (1.0 - recovery_factor));
    damage = clamp01(damage_dose / profile.damage_scale_s);
}

void add_supporting(ReasonRecord& reason, const std::string& name, const std::string& unit, double value) {
    if (std::isfinite(value)) reason.supporting_variables.push_back(SupportingVariable{name, unit, value});
}

void add_reason(
    PlantConsequencePresentation& presentation,
    const std::string& code,
    double severity,
    double start,
    double duration,
    const std::string& channel,
    const std::string& status
) {
    ReasonRecord reason;
    reason.reason_code = code;
    reason.severity_0_1 = clamp01(severity);
    reason.start_time_s = start;
    reason.duration_s = duration;
    reason.source_channel = channel;
    reason.status = status;
    presentation.reason_codes.push_back(std::move(reason));
}

void derive_presentation(
    PlantConsequenceState& state,
    PlantConsequencePresentation& presentation,
    const PlantConsequenceProfile& profile,
    const SegmentInputs& inputs
) {
    presentation = PlantConsequencePresentation{};
    presentation.growth_stage = state.stage;
    presentation.stage_progress_0_1 = state.stage_progress_0_1;
    presentation.size_proxy_0_1 = state.potential_opportunity_s > kEpsilon
        ? clamp01(state.achieved_opportunity_s / state.potential_opportunity_s)
        : 0.0;
    presentation.wilt_severity_0_1 = state.water_available
        ? std::max(state.water_current_severity_0_1, state.water_recovery_0_1) : 0.0;
    presentation.heat_stress_severity_0_1 = state.thermal_available
        ? std::max(state.thermal_current_severity_0_1, state.thermal_recovery_0_1) : 0.0;
    presentation.radiation_stress_severity_0_1 = state.radiation_available
        ? std::max(state.radiation_current_severity_0_1, state.radiation_recovery_0_1) : 0.0;
    presentation.damage_severity_0_1 = std::max({state.water_damage_0_1, state.thermal_damage_0_1, state.radiation_damage_0_1});
    presentation.recovery_severity_0_1 = std::max({state.water_recovery_0_1, state.thermal_recovery_0_1, state.radiation_recovery_0_1});
    presentation.growth_slowdown_0_1 = state.opportunity_available
        ? clamp01(1.0 - state.current_opportunity_signal_0_1) : 0.0;
    presentation.vigor_signal_0_1 = state.opportunity_available
        ? clamp01(state.current_opportunity_signal_0_1 * (1.0 - presentation.damage_severity_0_1)) : 0.0;

    const bool all_inputs_available = state.opportunity_available && state.water_available &&
        state.thermal_available && state.radiation_available;
    if (!profile.is_configured || !all_inputs_available) {
        presentation.derived_condition = "UNAVAILABLE";
        add_reason(presentation, "P2_INPUT_UNAVAILABLE", 1.0, inputs.start_time_s, inputs.duration_s,
                   "availability", state.integration_status);
    } else {
        const double active_max = std::max({state.water_current_severity_0_1,
                                            state.thermal_current_severity_0_1,
                                            state.radiation_current_severity_0_1});
        const bool channel_critical =
            (state.water_available && state.water_current_severity_0_1 >= profile.water.critical) ||
            (state.thermal_available && state.thermal_current_severity_0_1 >= profile.thermal.critical) ||
            (state.radiation_available && state.radiation_current_severity_0_1 >= profile.radiation.critical);
        if (channel_critical || active_max >= profile.presentation.critical_onset ||
            presentation.damage_severity_0_1 >= profile.presentation.critical_onset) {
            presentation.derived_condition = "CRITICAL";
        } else if (presentation.damage_severity_0_1 >= profile.presentation.damage_onset) {
            presentation.derived_condition = "DAMAGED";
        } else if (active_max < profile.presentation.stress_onset &&
                   presentation.recovery_severity_0_1 >= profile.presentation.recovery_onset) {
            presentation.derived_condition = "RECOVERING";
        } else if (active_max >= profile.presentation.stress_onset ||
                   state.current_opportunity_signal_0_1 < profile.presentation.thriving_opportunity) {
            presentation.derived_condition = "STRESSED";
        } else if (state.current_opportunity_signal_0_1 >= profile.presentation.thriving_opportunity) {
            presentation.derived_condition = "THRIVING";
        } else {
            presentation.derived_condition = "STABLE";
        }
    }

    const double reason_start = inputs.start_time_s;
    const double reason_duration = inputs.duration_s;
    if (state.water_available && state.water_current_severity_0_1 >= profile.presentation.stress_onset) {
        add_reason(presentation, "WATER_SUPPLY_LIMITED", state.water_current_severity_0_1, reason_start, reason_duration, "water", "available");
        add_supporting(presentation.reason_codes.back(), "water_current_severity_0_1", "1", state.water_current_severity_0_1);
        if (inputs.hydraulic_beta.has_value()) add_supporting(presentation.reason_codes.back(), "hydraulic_beta", "1", *inputs.hydraulic_beta);
        add_supporting(presentation.reason_codes.back(), "requested_water_mol", "mol H2O", inputs.requested_water_mol);
        add_supporting(presentation.reason_codes.back(), "realized_water_mol", "mol H2O", inputs.realized_water_mol);
        add_supporting(presentation.reason_codes.back(), "unmet_water_mol", "mol H2O", std::max(0.0, inputs.requested_water_mol - inputs.realized_water_mol));
    }
    if (state.thermal_available && state.thermal_current_severity_0_1 >= profile.presentation.stress_onset) {
        add_reason(presentation, "HIGH_LEAF_TEMPERATURE", state.thermal_current_severity_0_1, reason_start, reason_duration, "thermal", "available");
        add_supporting(presentation.reason_codes.back(), "leaf_temperature_c", "degC", inputs.leaf_temperature_c.value_or(0.0));
    }
    if (state.radiation_available && state.radiation_current_severity_0_1 >= profile.presentation.stress_onset) {
        add_reason(presentation, "HIGH_RADIATION_LOAD", state.radiation_current_severity_0_1, reason_start, reason_duration, "radiation", "available");
        add_supporting(presentation.reason_codes.back(), "absorbed_shortwave_w_m2", "W m^-2", inputs.absorbed_shortwave_w_m2.value_or(0.0));
    }
    if (state.opportunity_available && state.current_opportunity_signal_0_1 < profile.presentation.thriving_opportunity) {
        add_reason(presentation, "LOW_GROWTH_OPPORTUNITY", 1.0 - state.current_opportunity_signal_0_1,
                   reason_start, reason_duration, "opportunity", "available");
        add_supporting(presentation.reason_codes.back(), "opportunity_signal_0_1", "1", state.current_opportunity_signal_0_1);
    }
    if (state.water_recovery_0_1 >= profile.presentation.recovery_onset &&
        state.water_current_severity_0_1 < profile.presentation.stress_onset) {
        add_reason(presentation, "RECOVERING_FROM_WATER_STRESS", state.water_recovery_0_1,
                   reason_start, reason_duration, "water_recovery", "available");
    }
    if (state.thermal_recovery_0_1 >= profile.presentation.recovery_onset &&
        state.thermal_current_severity_0_1 < profile.presentation.stress_onset) {
        add_reason(presentation, "RECOVERING_FROM_THERMAL_STRESS", state.thermal_recovery_0_1,
                   reason_start, reason_duration, "thermal_recovery", "available");
    }
    if (state.radiation_recovery_0_1 >= profile.presentation.recovery_onset &&
        state.radiation_current_severity_0_1 < profile.presentation.stress_onset) {
        add_reason(presentation, "RECOVERING_FROM_RADIATION_STRESS", state.radiation_recovery_0_1,
                   reason_start, reason_duration, "radiation_recovery", "available");
    }
    if (state.water_damage_0_1 >= profile.presentation.damage_onset) add_reason(presentation, "PERSISTENT_WATER_DAMAGE", state.water_damage_0_1, reason_start, reason_duration, "water_damage", "available");
    if (state.thermal_damage_0_1 >= profile.presentation.damage_onset) add_reason(presentation, "PERSISTENT_THERMAL_DAMAGE", state.thermal_damage_0_1, reason_start, reason_duration, "thermal_damage", "available");
    if (state.radiation_damage_0_1 >= profile.presentation.damage_onset) add_reason(presentation, "PERSISTENT_RADIATION_DAMAGE", state.radiation_damage_0_1, reason_start, reason_duration, "radiation_damage", "available");
    if (state.lost_opportunity_s > kEpsilon && state.stage_progress_0_1 >= 1.0 - kEpsilon) {
        add_reason(presentation, "STAGE_DELAYED_BY_LOST_OPPORTUNITY", 1.0, reason_start, reason_duration, "lost_opportunity", "available");
    }
}

} // namespace

std::string to_string(DevelopmentStage stage) {
    switch (stage) {
        case DevelopmentStage::Seedling: return "seedling";
        case DevelopmentStage::Vegetative: return "vegetative";
        case DevelopmentStage::Flowering: return "flowering";
        case DevelopmentStage::HarvestReady: return "harvest_ready";
    }
    return "unknown";
}

DevelopmentStage development_stage_from_string(const std::string& value) {
    if (value == "seedling") return DevelopmentStage::Seedling;
    if (value == "vegetative") return DevelopmentStage::Vegetative;
    if (value == "flowering") return DevelopmentStage::Flowering;
    if (value == "harvest_ready" || value == "harvest") return DevelopmentStage::HarvestReady;
    throw std::invalid_argument("unknown consequence development stage: " + value);
}

PlantConsequenceProfile get_unconfigured_profile() {
    return PlantConsequenceProfile{};
}

PlantConsequenceProfile get_synthetic_test_profile() {
    PlantConsequenceProfile profile;
    profile.profile_id = "synthetic_p2_test";
    profile.model_version = "p2_growth_consequences.synthetic_test.v1";
    profile.source = "engineering fixture only";
    profile.evidence_classification = "SYNTHETIC_TEST_VALUE";
    profile.calibration_status = "SYNTHETIC_TEST_VALUE";
    profile.valid_domain = "automated tests only";
    profile.is_configured = true;
    profile.stages = {
        {DevelopmentStage::Seedling, 86400.0, 10.0, std::nullopt, std::nullopt},
        {DevelopmentStage::Vegetative, 86400.0, 10.0, std::nullopt, std::nullopt},
        {DevelopmentStage::Flowering, 86400.0, 10.0, std::nullopt, std::nullopt},
        {DevelopmentStage::HarvestReady, 1.0, 10.0, std::nullopt, std::nullopt},
    };
    profile.water = {0.2, 0.8, 0.9, 1.0 / 3600.0, 0.4, 3600.0};
    profile.thermal = {30.0, 40.0, 0.9, 1.0 / 3600.0, 0.4, 3600.0};
    profile.radiation = {500.0, 1000.0, 0.9, 1.0 / 3600.0, 0.4, 3600.0};
    profile.presentation = {0.2, 0.2, 0.2, 0.9, 0.8};
    return profile;
}

std::vector<std::string> validate_profile(const PlantConsequenceProfile& profile) {
    std::vector<std::string> errors;
    if (!profile.is_configured) {
        errors.push_back("consequence profile is not configured");
        return errors;
    }
    if (profile.profile_id.empty()) errors.push_back("consequence profile_id must not be empty");
    if (profile.model_version.empty()) errors.push_back("consequence model_version must not be empty");
    if (profile.source.empty()) errors.push_back("consequence source must not be empty");
    if (profile.evidence_classification.empty()) errors.push_back("consequence evidence classification must not be empty");
    if (profile.calibration_status.empty()) errors.push_back("consequence calibration status must not be empty");
    if (profile.valid_domain.empty()) errors.push_back("consequence valid domain must not be empty");
    if (profile.stages.size() != 4) errors.push_back("consequence profile requires four stages");
    const DevelopmentStage expected_stages[] = {
        DevelopmentStage::Seedling,
        DevelopmentStage::Vegetative,
        DevelopmentStage::Flowering,
        DevelopmentStage::HarvestReady,
    };
    if (profile.stages.size() == 4) {
        for (std::size_t index = 0; index < profile.stages.size(); ++index) {
            if (profile.stages[index].stage != expected_stages[index]) {
                errors.push_back("consequence stages must be ordered seedling, vegetative, flowering, harvest_ready");
                break;
            }
        }
    }
    for (const StageProfile& stage : profile.stages) {
        if (!finite_nonnegative(stage.required_opportunity_s) || stage.required_opportunity_s <= 0.0) errors.push_back("stage opportunity requirement must be positive");
        if (!finite_nonnegative(stage.assimilation_reference_umol_m2_s) || stage.assimilation_reference_umol_m2_s <= 0.0) errors.push_back("stage assimilation reference must be positive");
        if (stage.minimum_elapsed_stage_s.has_value() && !finite_nonnegative(*stage.minimum_elapsed_stage_s)) errors.push_back("minimum stage time must be non-negative");
        if (stage.minimum_dark_duration_s.has_value() && !finite_nonnegative(*stage.minimum_dark_duration_s)) errors.push_back("minimum dark duration must be non-negative");
    }
    const auto validate_channel = [&errors](const ExposureProfile& channel, const char* name) {
        if (!std::isfinite(channel.onset) || !std::isfinite(channel.full) || channel.full <= channel.onset) errors.push_back(std::string(name) + " onset/full range invalid");
        if (!std::isfinite(channel.critical) || channel.critical < 0.0 || channel.critical > 1.0) errors.push_back(std::string(name) + " critical severity must be within [0,1]");
        if (!std::isfinite(channel.recovery_rate_per_s) || channel.recovery_rate_per_s < 0.0) errors.push_back(std::string(name) + " recovery rate invalid");
        if (!std::isfinite(channel.damage_onset) || channel.damage_onset < 0.0 || channel.damage_onset > 1.0) errors.push_back(std::string(name) + " damage onset invalid");
        if (!finite_nonnegative(channel.damage_scale_s) || channel.damage_scale_s <= 0.0) errors.push_back(std::string(name) + " damage scale invalid");
    };
    validate_channel(profile.water, "water");
    validate_channel(profile.thermal, "thermal");
    validate_channel(profile.radiation, "radiation");
    const auto validate_presentation = [&errors](const PresentationThresholds& presentation) {
        const double values[] = {
            presentation.stress_onset,
            presentation.recovery_onset,
            presentation.damage_onset,
            presentation.critical_onset,
            presentation.thriving_opportunity,
        };
        for (double value : values) {
            if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
                errors.push_back("presentation thresholds must be finite and within [0,1]");
                break;
            }
        }
    };
    validate_presentation(profile.presentation);
    return errors;
}

void integrate_segment(
    PlantConsequenceState& state,
    PlantConsequencePresentation& presentation,
    const PlantConsequenceProfile& profile,
    const SegmentInputs& inputs
) {
    if (!finite_nonnegative(inputs.duration_s)) throw std::invalid_argument("P2 segment duration must be finite and non-negative");
    state.model_version = profile.model_version;
    state.last_integrated_simulation_time_s = inputs.start_time_s + inputs.duration_s;
    const auto errors = validate_profile(profile);
    if (!errors.empty()) {
        state.integration_status = profile.is_configured ? "profile_invalid" : "profile_unconfigured";
        state.opportunity_available = false;
        state.water_available = false;
        state.thermal_available = false;
        state.radiation_available = false;
        state.current_opportunity_signal_0_1 = 0.0;
        state.water_current_severity_0_1 = 0.0;
        state.thermal_current_severity_0_1 = 0.0;
        state.radiation_current_severity_0_1 = 0.0;
        derive_presentation(state, presentation, profile, inputs);
        return;
    }

    const StageProfile* current_profile = stage_profile_for(profile, state.stage);
    if (current_profile == nullptr) {
        state.integration_status = "stage_profile_unavailable";
        state.opportunity_available = false;
        state.current_opportunity_signal_0_1 = 0.0;
        derive_presentation(state, presentation, profile, inputs);
        return;
    }

    const bool assimilation_available = inputs.net_assimilation_umol_m2_s.has_value() &&
        std::isfinite(*inputs.net_assimilation_umol_m2_s) &&
        inputs.gas_exchange_status == "Converged";
    const bool water_fulfilment_available = inputs.water_allocation_available &&
        finite_nonnegative(inputs.requested_water_mol) && finite_nonnegative(inputs.realized_water_mol);
    const bool opportunity_available = assimilation_available && water_fulfilment_available;
    state.opportunity_available = opportunity_available;
    state.current_opportunity_signal_0_1 = 0.0;

    const double water_fulfilment = opportunity_available
        ? (inputs.requested_water_mol > 0.0
            ? clamp01(inputs.realized_water_mol / inputs.requested_water_mol) : 1.0)
        : 0.0;

    const bool beta_available = inputs.hydraulic_beta.has_value() && std::isfinite(*inputs.hydraulic_beta);
    const bool shortage_available = water_fulfilment_available;
    if (beta_available || shortage_available) {
        const double beta_severity = beta_available ? clamp01(1.0 - *inputs.hydraulic_beta) : 0.0;
        const double shortage_severity = shortage_available && inputs.requested_water_mol > 0.0
            ? clamp01((inputs.requested_water_mol - inputs.realized_water_mol) / inputs.requested_water_mol) : 0.0;
        update_channel(std::max(beta_severity, shortage_severity), profile.water, inputs.duration_s,
                       state.water_current_severity_0_1, state.water_available,
                       state.water_exposure_dose_s, state.water_damage_dose_s,
                       state.water_recovery_0_1, state.water_damage_0_1);
    } else {
        state.water_available = false;
        state.water_current_severity_0_1 = 0.0;
    }

    if (inputs.leaf_temperature_c.has_value() && std::isfinite(*inputs.leaf_temperature_c)) {
        const double severity = map_exposure(*inputs.leaf_temperature_c, profile.thermal);
        if (std::isfinite(severity)) update_channel(severity, profile.thermal, inputs.duration_s,
            state.thermal_current_severity_0_1, state.thermal_available,
            state.thermal_exposure_dose_s, state.thermal_damage_dose_s,
            state.thermal_recovery_0_1, state.thermal_damage_0_1);
    } else {
        state.thermal_available = false;
        state.thermal_current_severity_0_1 = 0.0;
    }

    if (inputs.absorbed_shortwave_w_m2.has_value() && std::isfinite(*inputs.absorbed_shortwave_w_m2)) {
        const double severity = map_exposure(*inputs.absorbed_shortwave_w_m2, profile.radiation);
        if (std::isfinite(severity)) update_channel(severity, profile.radiation, inputs.duration_s,
            state.radiation_current_severity_0_1, state.radiation_available,
            state.radiation_exposure_dose_s, state.radiation_damage_dose_s,
            state.radiation_recovery_0_1, state.radiation_damage_0_1);
    } else {
        state.radiation_available = false;
        state.radiation_current_severity_0_1 = 0.0;
    }

    if (!opportunity_available) {
        state.integration_status = "opportunity_unavailable";
    } else if (!state.water_available || !state.thermal_available || !state.radiation_available) {
        state.integration_status = "partial_unavailable";
    } else {
        state.integration_status = "available";
    }

    double remaining = inputs.duration_s;
    while (remaining > kEpsilon && opportunity_available) {
        current_profile = stage_profile_for(profile, state.stage);
        if (current_profile == nullptr) break;
        const double assimilation_fraction = clamp01(
            std::max(0.0, *inputs.net_assimilation_umol_m2_s) /
            current_profile->assimilation_reference_umol_m2_s);
        const double opportunity_signal = std::min(assimilation_fraction, water_fulfilment);
        state.current_opportunity_signal_0_1 = opportunity_signal;
        const auto integrate_opportunity = [&](double interval_s) {
            state.potential_opportunity_s += interval_s;
            state.achieved_opportunity_s += opportunity_signal * interval_s;
            state.lost_opportunity_s += (1.0 - opportunity_signal) * interval_s;
        };
        if (state.stage == DevelopmentStage::HarvestReady) {
            integrate_opportunity(remaining);
            state.stage_elapsed_s += remaining;
            remaining = 0.0;
            break;
        }
        const double required = current_profile->required_opportunity_s;
        const double opportunity_remaining = std::max(0.0, required - state.stage_opportunity_accumulated_s);
        const double time_to_opportunity = opportunity_signal > kEpsilon
            ? opportunity_remaining / opportunity_signal : std::numeric_limits<double>::infinity();
        const double time_to_minimum = current_profile->minimum_elapsed_stage_s.has_value()
            ? std::max(0.0, *current_profile->minimum_elapsed_stage_s - state.stage_elapsed_s)
            : 0.0;
        const bool photoperiod_gate = current_profile->minimum_dark_duration_s.has_value() &&
            inputs.accumulated_dark_duration_s < *current_profile->minimum_dark_duration_s;
        if (photoperiod_gate) {
            integrate_opportunity(remaining);
            state.stage_opportunity_accumulated_s = std::min(required,
                state.stage_opportunity_accumulated_s + opportunity_signal * remaining);
            state.stage_elapsed_s += remaining;
            remaining = 0.0;
            break;
        }
        const double transition_time = std::max(time_to_opportunity, time_to_minimum);
        if (!std::isfinite(transition_time) || transition_time > remaining + kEpsilon) {
            integrate_opportunity(remaining);
            state.stage_opportunity_accumulated_s = std::min(required,
                state.stage_opportunity_accumulated_s + opportunity_signal * remaining);
            state.stage_elapsed_s += remaining;
            remaining = 0.0;
            break;
        }
        const double consumed = std::max(0.0, transition_time);
        integrate_opportunity(consumed);
        state.stage_opportunity_accumulated_s = std::min(required,
            state.stage_opportunity_accumulated_s + opportunity_signal * consumed);
        state.stage_elapsed_s += consumed;
        remaining -= consumed;
        state.stage = next_stage(state.stage);
        state.stage_opportunity_accumulated_s = 0.0;
        state.stage_elapsed_s = 0.0;
    }
    if (state.stage == DevelopmentStage::HarvestReady && remaining > kEpsilon) {
        state.stage_progress_0_1 = 1.0;
        state.stage_elapsed_s += remaining;
        remaining = 0.0;
    } else if (!opportunity_available) {
        state.stage_elapsed_s += inputs.duration_s;
    }
    current_profile = stage_profile_for(profile, state.stage);
    state.stage_progress_0_1 = state.stage == DevelopmentStage::HarvestReady
        ? 1.0
        : (current_profile != nullptr ? clamp01(state.stage_opportunity_accumulated_s / current_profile->required_opportunity_s) : 0.0);

    derive_presentation(state, presentation, profile, inputs);
}

} // namespace cannaville::consequences
