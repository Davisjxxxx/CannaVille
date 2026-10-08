#include "Simulation/RootZone/RootZoneWater.hpp"
#include <cmath>

namespace cannaville::rootzone {

// 1 mole of water = 18.01528 g. Density of water = 998.2 kg/m^3 (approx, standard).
// 1 mol = 0.01801528 kg / 998.2 kg/m^3 = 1.80477e-5 m^3
// Alternatively, standard density of 1000 kg/m^3 -> 1.801528e-5 m^3.
constexpr double MOLES_TO_M3_WATER = 1.801528e-5;

double moles_water_to_cubic_meters(double moles) {
    return moles * MOLES_TO_M3_WATER;
}

double cubic_meters_water_to_moles(double cubic_meters) {
    return cubic_meters / MOLES_TO_M3_WATER;
}

SubstrateContainer::SubstrateContainer(units::VolumeCubicMeters max_capacity, units::VolumeCubicMeters initial_water)
    : max_capacity_{max_capacity}, current_water_{initial_water} {
    reset_balance();
}

void SubstrateContainer::reset_balance() {
    last_balance_ = MassBalanceResult{};
    last_balance_.initial_water_volume_m3 = current_water_.value;
    last_balance_.final_water_volume_m3 = current_water_.value;
}

void SubstrateContainer::irrigate(units::VolumeCubicMeters amount) {
    if (amount.value > 0.0) {
        current_water_.value += amount.value;
        last_balance_.irrigation_added_m3 += amount.value;
        
        // Update residual
        last_balance_.final_water_volume_m3 = current_water_.value;
        last_balance_.residual_m3 = last_balance_.final_water_volume_m3 - 
            (last_balance_.initial_water_volume_m3 + last_balance_.irrigation_added_m3 - 
             last_balance_.plant_withdrawal_m3 - last_balance_.drainage_removed_m3);
    }
}

units::VolumeCubicMeters SubstrateContainer::withdraw_transpiration(
    units::WaterFluxMolesPerSquareMeterSecond transpiration_flux,
    units::AreaSquareMeters effective_transpiring_leaf_area,
    units::Seconds delta_time) {
    
    // QUASI_STEADY_ROOT_UPTAKE_APPROXIMATION
    double moles_withdrawn = transpiration_flux.value * effective_transpiring_leaf_area.value * delta_time.value;
    double volume_withdrawn_m3 = moles_water_to_cubic_meters(moles_withdrawn);
    
    double actual_withdrawn = volume_withdrawn_m3;
    if (current_water_.value < volume_withdrawn_m3) {
        last_balance_.status = WaterStatus::InsufficientRootzoneWater;
        actual_withdrawn = current_water_.value;
        current_water_.value = 0.0;
    } else {
        current_water_.value -= actual_withdrawn;
    }
    
    last_balance_.plant_withdrawal_m3 += actual_withdrawn;
    last_balance_.final_water_volume_m3 = current_water_.value;
    last_balance_.residual_m3 = last_balance_.final_water_volume_m3 - 
        (last_balance_.initial_water_volume_m3 + last_balance_.irrigation_added_m3 - 
         last_balance_.plant_withdrawal_m3 - last_balance_.drainage_removed_m3);
         
    return units::VolumeCubicMeters{actual_withdrawn};
}

void SubstrateContainer::process_drainage() {
    // STORAGE_CAPACITY_OVERFLOW_APPROXIMATION
    if (current_water_.value > max_capacity_.value) {
        double drainage = current_water_.value - max_capacity_.value;
        current_water_.value = max_capacity_.value;
        last_balance_.drainage_removed_m3 += drainage;
        last_balance_.status = WaterStatus::Overflow;
        
        last_balance_.final_water_volume_m3 = current_water_.value;
        last_balance_.residual_m3 = last_balance_.final_water_volume_m3 - 
            (last_balance_.initial_water_volume_m3 + last_balance_.irrigation_added_m3 - 
             last_balance_.plant_withdrawal_m3 - last_balance_.drainage_removed_m3);
    }
}

double SubstrateContainer::volumetric_water_content() const {
    if (max_capacity_.value <= 0.0) return 0.0;
    return current_water_.value / max_capacity_.value;
}

HydroponicReservoir::HydroponicReservoir(units::VolumeCubicMeters max_capacity, units::VolumeCubicMeters initial_water)
    : max_capacity_{max_capacity}, current_water_{initial_water} {
    reset_balance();
}

void HydroponicReservoir::reset_balance() {
    last_balance_ = MassBalanceResult{};
    last_balance_.initial_water_volume_m3 = current_water_.value;
    last_balance_.final_water_volume_m3 = current_water_.value;
}

void HydroponicReservoir::add_water(units::VolumeCubicMeters amount) {
    if (amount.value > 0.0) {
        current_water_.value += amount.value;
        last_balance_.irrigation_added_m3 += amount.value;
        
        last_balance_.final_water_volume_m3 = current_water_.value;
        last_balance_.residual_m3 = last_balance_.final_water_volume_m3 - 
            (last_balance_.initial_water_volume_m3 + last_balance_.irrigation_added_m3 - 
             last_balance_.plant_withdrawal_m3 - last_balance_.drainage_removed_m3);
    }
}

units::VolumeCubicMeters HydroponicReservoir::withdraw_transpiration(
    units::WaterFluxMolesPerSquareMeterSecond transpiration_flux,
    units::AreaSquareMeters effective_transpiring_leaf_area,
    units::Seconds delta_time) {
    
    // QUASI_STEADY_ROOT_UPTAKE_APPROXIMATION
    double moles_withdrawn = transpiration_flux.value * effective_transpiring_leaf_area.value * delta_time.value;
    double volume_withdrawn_m3 = moles_water_to_cubic_meters(moles_withdrawn);
    
    double actual_withdrawn = volume_withdrawn_m3;
    if (current_water_.value < volume_withdrawn_m3) {
        last_balance_.status = WaterStatus::InsufficientRootzoneWater;
        actual_withdrawn = current_water_.value;
        current_water_.value = 0.0;
    } else {
        current_water_.value -= actual_withdrawn;
    }
    
    last_balance_.plant_withdrawal_m3 += actual_withdrawn;
    last_balance_.final_water_volume_m3 = current_water_.value;
    last_balance_.residual_m3 = last_balance_.final_water_volume_m3 - 
        (last_balance_.initial_water_volume_m3 + last_balance_.irrigation_added_m3 - 
         last_balance_.plant_withdrawal_m3 - last_balance_.drainage_removed_m3);
         
    return units::VolumeCubicMeters{actual_withdrawn};
}

void HydroponicReservoir::process_overflow() {
    // STORAGE_CAPACITY_OVERFLOW_APPROXIMATION
    if (current_water_.value > max_capacity_.value) {
        double overflow = current_water_.value - max_capacity_.value;
        current_water_.value = max_capacity_.value;
        last_balance_.drainage_removed_m3 += overflow;
        last_balance_.status = WaterStatus::Overflow;
        
        last_balance_.final_water_volume_m3 = current_water_.value;
        last_balance_.residual_m3 = last_balance_.final_water_volume_m3 - 
            (last_balance_.initial_water_volume_m3 + last_balance_.irrigation_added_m3 - 
             last_balance_.plant_withdrawal_m3 - last_balance_.drainage_removed_m3);
    }
}

} // namespace cannaville::rootzone
