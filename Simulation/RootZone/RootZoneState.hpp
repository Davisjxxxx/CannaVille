#pragma once

#include "Simulation/Core/Units.hpp"
#include <string>
#include <optional>

namespace cannaville::rootzone {

enum class RootZoneType {
    Substrate,
    Reservoir
};

struct RootZoneState {
    std::string id{"rootzone_001"};
    RootZoneType type{RootZoneType::Substrate};
    
    // Core parameters
    units::VolumeCubicMeters substrate_bulk_volume{};
    std::optional<units::VolumeCubicMeters> max_stored_water{};
    std::optional<std::string> substrate_hydraulic_profile_id;
    std::optional<std::string> hydraulic_stress_transfer_profile_id;
    std::optional<bool> explicit_unrestricted_water_access;
    
    // Dynamic physical state
    units::VolumeCubicMeters current_water_volume{};
    
    // Derived (computed on read/write or strictly physical)
    std::optional<double> volumetric_water_content{};
    std::optional<double> storage_fraction{};
    
    // Cumulative ledger
    units::VolumeCubicMeters cumulative_irrigation_top_off{};
    units::VolumeCubicMeters cumulative_external_return_flow{};
    units::VolumeCubicMeters cumulative_realized_withdrawal{};
    units::VolumeCubicMeters cumulative_drainage_discharge{};
    units::VolumeCubicMeters cumulative_evaporation{};
    units::VolumeCubicMeters cumulative_unmet_demand{};
    
    // Legacy placeholders
    units::TemperatureCelsius root_zone_temperature{};
    units::PH ph{};
    units::EcmilliSiemensPerCentimeter electrical_conductivity{};
};

} // namespace cannaville::rootzone
