#include "Simulation/RootZone/RootZoneWater.hpp"
#include <cmath>
#include <stdexcept>

namespace cannaville::rootzone {

// Conversion constants
// Molar mass of water = 18.01528 g/mol = 0.01801528 kg/mol
// Density of liquid water = 998.2 kg/m^3 (approximate standard reference temperature 20C)
// 1 mol = 0.01801528 kg / 998.2 kg/m^3 = 1.8047766e-5 m^3
constexpr double MOLES_TO_M3_WATER = 1.8047766e-5;

double moles_water_to_cubic_meters(double moles) {
    return moles * MOLES_TO_M3_WATER;
}

double cubic_meters_water_to_moles(double cubic_meters) {
    return cubic_meters / MOLES_TO_M3_WATER;
}

SubstrateContainer::SubstrateContainer(
    units::VolumeCubicMeters substrate_bulk_volume, 
    units::VolumeCubicMeters initial_water,
    std::optional<units::VolumeCubicMeters> max_stored_water)
    : substrate_bulk_volume_{substrate_bulk_volume}, 
      max_stored_water_{max_stored_water}, 
      current_water_{initial_water} {
      
    if (substrate_bulk_volume_.value <= 0.0) {
        throw std::invalid_argument("Substrate bulk volume must be strictly positive");
    }
    if (current_water_.value < 0.0) {
        throw std::invalid_argument("Initial water volume cannot be negative");
    }
      
    reset_balance();
}

void SubstrateContainer::reset_balance() {
    last_balance_ = MassBalanceResult{};
    last_balance_.initial_water_volume_m3 = current_water_.value;
    last_balance_.final_water_volume_m3 = current_water_.value;
}

void SubstrateContainer::update_residual() {
    last_balance_.final_water_volume_m3 = current_water_.value;
    double inputs = last_balance_.irrigation_added_m3 + last_balance_.top_off_added_m3 + last_balance_.external_return_flow_added_m3;
    double outputs = last_balance_.plant_withdrawal_realized_m3 + last_balance_.drainage_removed_m3 + last_balance_.evaporation_removed_m3;
    last_balance_.residual_m3 = last_balance_.final_water_volume_m3 - (last_balance_.initial_water_volume_m3 + inputs - outputs);
}

void SubstrateContainer::irrigate(units::VolumeCubicMeters amount) {
    if (amount.value > 0.0) {
        current_water_.value += amount.value;
        last_balance_.irrigation_added_m3 += amount.value;
        update_residual();
    }
}

void SubstrateContainer::add_top_off(units::VolumeCubicMeters amount) {
    if (amount.value > 0.0) {
        current_water_.value += amount.value;
        last_balance_.top_off_added_m3 += amount.value;
        update_residual();
    }
}

void SubstrateContainer::add_external_return_flow(units::VolumeCubicMeters amount) {
    if (amount.value > 0.0) {
        current_water_.value += amount.value;
        last_balance_.external_return_flow_added_m3 += amount.value;
        update_residual();
    }
}

void SubstrateContainer::evaporate(units::VolumeCubicMeters amount) {
    if (amount.value > 0.0) {
        double actual = std::min(amount.value, current_water_.value);
        current_water_.value -= actual;
        last_balance_.evaporation_removed_m3 += actual;
        update_residual();
    }
}

units::VolumeCubicMeters SubstrateContainer::withdraw_transpiration(
    units::WaterFluxMolesPerSquareMeterSecond transpiration_flux,
    units::AreaSquareMeters effective_transpiring_leaf_area,
    units::Seconds delta_time) {
    
    double moles_withdrawn = transpiration_flux.value * effective_transpiring_leaf_area.value * delta_time.value;
    double requested_m3 = moles_water_to_cubic_meters(moles_withdrawn);
    
    last_balance_.plant_withdrawal_requested_m3 += requested_m3;
    
    double actual_withdrawn = requested_m3;
    if (current_water_.value < requested_m3) {
        last_balance_.status = WaterStatus::InsufficientRootzoneWater;
        actual_withdrawn = current_water_.value;
        current_water_.value = 0.0;
    } else {
        current_water_.value -= actual_withdrawn;
    }
    
    double unmet = requested_m3 - actual_withdrawn;
    last_balance_.plant_withdrawal_realized_m3 += actual_withdrawn;
    last_balance_.plant_withdrawal_unmet_m3 += unmet;
    
    update_residual();
    return units::VolumeCubicMeters{actual_withdrawn};
}

std::vector<units::VolumeCubicMeters> SubstrateContainer::withdraw_transpiration_shared(
    const std::vector<TranspirationRequest>& requests,
    units::Seconds delta_time) {
    
    std::vector<double> requested_volumes;
    double total_requested = 0.0;
    
    for (const auto& req : requests) {
        double moles = req.flux.value * req.area.value * delta_time.value;
        double vol = moles_water_to_cubic_meters(moles);
        requested_volumes.push_back(vol);
        total_requested += vol;
    }
    
    last_balance_.plant_withdrawal_requested_m3 += total_requested;
    
    std::vector<units::VolumeCubicMeters> realized_volumes;
    if (total_requested <= current_water_.value || total_requested == 0.0) {
        // Adequate water
        for (double req : requested_volumes) {
            current_water_.value -= req;
            last_balance_.plant_withdrawal_realized_m3 += req;
            realized_volumes.push_back(units::VolumeCubicMeters{req});
        }
    } else {
        // Proportional shortage allocation
        last_balance_.status = WaterStatus::InsufficientRootzoneWater;
        double available = current_water_.value;
        current_water_.value = 0.0;
        
        for (double req : requested_volumes) {
            double proportion = req / total_requested;
            double actual = available * proportion;
            last_balance_.plant_withdrawal_realized_m3 += actual;
            last_balance_.plant_withdrawal_unmet_m3 += (req - actual);
            realized_volumes.push_back(units::VolumeCubicMeters{actual});
        }
    }
    
    update_residual();
    return realized_volumes;
}

void SubstrateContainer::process_drainage(std::optional<units::VolumeCubicMeters> explicit_drainage) {
    if (explicit_drainage.has_value()) {
        double actual = std::min(explicit_drainage->value, current_water_.value);
        current_water_.value -= actual;
        last_balance_.drainage_removed_m3 += actual;
    } else if (max_stored_water_.has_value()) {
        if (current_water_.value > max_stored_water_->value) {
            double drainage = current_water_.value - max_stored_water_->value;
            current_water_.value = max_stored_water_->value;
            last_balance_.drainage_removed_m3 += drainage;
            last_balance_.status = WaterStatus::Overflow;
        }
    }
    update_residual();
}

double SubstrateContainer::volumetric_water_content() const {
    return current_water_.value / substrate_bulk_volume_.value;
}

std::optional<double> SubstrateContainer::storage_fraction() const {
    if (!max_stored_water_.has_value() || max_stored_water_->value <= 0.0) {
        return std::nullopt;
    }
    return current_water_.value / max_stored_water_->value;
}

// HydroponicReservoir implementation

HydroponicReservoir::HydroponicReservoir(
    units::VolumeCubicMeters max_stored_water, 
    units::VolumeCubicMeters initial_water)
    : max_stored_water_{max_stored_water}, current_water_{initial_water} {
    if (max_stored_water_.value <= 0.0) {
        throw std::invalid_argument("Reservoir capacity must be positive");
    }
    if (current_water_.value < 0.0) {
        throw std::invalid_argument("Initial water volume cannot be negative");
    }
    reset_balance();
}

void HydroponicReservoir::reset_balance() {
    last_balance_ = MassBalanceResult{};
    last_balance_.initial_water_volume_m3 = current_water_.value;
    last_balance_.final_water_volume_m3 = current_water_.value;
}

void HydroponicReservoir::update_residual() {
    last_balance_.final_water_volume_m3 = current_water_.value;
    double inputs = last_balance_.irrigation_added_m3 + last_balance_.top_off_added_m3 + last_balance_.external_return_flow_added_m3;
    double outputs = last_balance_.plant_withdrawal_realized_m3 + last_balance_.discharge_removed_m3 + last_balance_.evaporation_removed_m3;
    last_balance_.residual_m3 = last_balance_.final_water_volume_m3 - (last_balance_.initial_water_volume_m3 + inputs - outputs);
}

void HydroponicReservoir::add_top_off(units::VolumeCubicMeters amount) {
    if (amount.value > 0.0) {
        current_water_.value += amount.value;
        last_balance_.top_off_added_m3 += amount.value;
        update_residual();
    }
}

void HydroponicReservoir::add_external_return_flow(units::VolumeCubicMeters amount) {
    if (amount.value > 0.0) {
        current_water_.value += amount.value;
        last_balance_.external_return_flow_added_m3 += amount.value;
        update_residual();
    }
}

void HydroponicReservoir::evaporate(units::VolumeCubicMeters amount) {
    if (amount.value > 0.0) {
        double actual = std::min(amount.value, current_water_.value);
        current_water_.value -= actual;
        last_balance_.evaporation_removed_m3 += actual;
        update_residual();
    }
}

units::VolumeCubicMeters HydroponicReservoir::withdraw_transpiration(
    units::WaterFluxMolesPerSquareMeterSecond transpiration_flux,
    units::AreaSquareMeters effective_transpiring_leaf_area,
    units::Seconds delta_time) {
    
    double moles_withdrawn = transpiration_flux.value * effective_transpiring_leaf_area.value * delta_time.value;
    double requested_m3 = moles_water_to_cubic_meters(moles_withdrawn);
    
    last_balance_.plant_withdrawal_requested_m3 += requested_m3;
    
    double actual_withdrawn = requested_m3;
    if (current_water_.value < requested_m3) {
        last_balance_.status = WaterStatus::InsufficientRootzoneWater;
        actual_withdrawn = current_water_.value;
        current_water_.value = 0.0;
    } else {
        current_water_.value -= actual_withdrawn;
    }
    
    double unmet = requested_m3 - actual_withdrawn;
    last_balance_.plant_withdrawal_realized_m3 += actual_withdrawn;
    last_balance_.plant_withdrawal_unmet_m3 += unmet;
    
    update_residual();
    return units::VolumeCubicMeters{actual_withdrawn};
}

std::vector<units::VolumeCubicMeters> HydroponicReservoir::withdraw_transpiration_shared(
    const std::vector<TranspirationRequest>& requests,
    units::Seconds delta_time) {
    
    std::vector<double> requested_volumes;
    double total_requested = 0.0;
    
    for (const auto& req : requests) {
        double moles = req.flux.value * req.area.value * delta_time.value;
        double vol = moles_water_to_cubic_meters(moles);
        requested_volumes.push_back(vol);
        total_requested += vol;
    }
    
    last_balance_.plant_withdrawal_requested_m3 += total_requested;
    
    std::vector<units::VolumeCubicMeters> realized_volumes;
    if (total_requested <= current_water_.value || total_requested == 0.0) {
        // Adequate water
        for (double req : requested_volumes) {
            current_water_.value -= req;
            last_balance_.plant_withdrawal_realized_m3 += req;
            realized_volumes.push_back(units::VolumeCubicMeters{req});
        }
    } else {
        // Proportional shortage allocation
        last_balance_.status = WaterStatus::InsufficientRootzoneWater;
        double available = current_water_.value;
        current_water_.value = 0.0;
        
        for (double req : requested_volumes) {
            double proportion = req / total_requested;
            double actual = available * proportion;
            last_balance_.plant_withdrawal_realized_m3 += actual;
            last_balance_.plant_withdrawal_unmet_m3 += (req - actual);
            realized_volumes.push_back(units::VolumeCubicMeters{actual});
        }
    }
    
    update_residual();
    return realized_volumes;
}

void HydroponicReservoir::process_discharge(std::optional<units::VolumeCubicMeters> explicit_discharge) {
    if (explicit_discharge.has_value()) {
        double actual = std::min(explicit_discharge->value, current_water_.value);
        current_water_.value -= actual;
        last_balance_.discharge_removed_m3 += actual;
    } else {
        if (current_water_.value > max_stored_water_.value) {
            double overflow = current_water_.value - max_stored_water_.value;
            current_water_.value = max_stored_water_.value;
            last_balance_.discharge_removed_m3 += overflow;
            last_balance_.status = WaterStatus::Overflow;
        }
    }
    update_residual();
}

double HydroponicReservoir::storage_fraction() const {
    return current_water_.value / max_stored_water_.value;
}

} // namespace cannaville::rootzone
