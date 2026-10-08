#pragma once

#include "Simulation/Core/Units.hpp"
#include <string>

namespace cannaville::rootzone {

enum class WaterStatus {
    Adequate,
    InsufficientRootzoneWater,
    Overflow
};

struct MassBalanceResult {
    double initial_water_volume_m3{0.0};
    double final_water_volume_m3{0.0};
    double irrigation_added_m3{0.0};
    double drainage_removed_m3{0.0};
    double plant_withdrawal_m3{0.0};
    double residual_m3{0.0}; // Should be very close to 0
    WaterStatus status{WaterStatus::Adequate};
};

class SubstrateContainer {
public:
    SubstrateContainer(units::VolumeCubicMeters max_capacity, units::VolumeCubicMeters initial_water);

    void irrigate(units::VolumeCubicMeters amount);
    
    // Returns actual volume withdrawn. Uses QUASI_STEADY_ROOT_UPTAKE_APPROXIMATION.
    units::VolumeCubicMeters withdraw_transpiration(
        units::WaterFluxMolesPerSquareMeterSecond transpiration_flux,
        units::AreaSquareMeters effective_transpiring_leaf_area,
        units::Seconds delta_time);

    void process_drainage();

    units::VolumeCubicMeters current_water() const { return current_water_; }
    units::VolumeCubicMeters max_capacity() const { return max_capacity_; }
    
    // Physical Volumetric Water Content calculation
    double volumetric_water_content() const;
    
    MassBalanceResult get_last_balance() const { return last_balance_; }
    void reset_balance();

private:
    units::VolumeCubicMeters max_capacity_{};
    units::VolumeCubicMeters current_water_{};
    MassBalanceResult last_balance_{};
};

class HydroponicReservoir {
public:
    HydroponicReservoir(units::VolumeCubicMeters max_capacity, units::VolumeCubicMeters initial_water);

    void add_water(units::VolumeCubicMeters amount);
    
    // Returns actual volume withdrawn. Uses QUASI_STEADY_ROOT_UPTAKE_APPROXIMATION.
    units::VolumeCubicMeters withdraw_transpiration(
        units::WaterFluxMolesPerSquareMeterSecond transpiration_flux,
        units::AreaSquareMeters effective_transpiring_leaf_area,
        units::Seconds delta_time);

    void process_overflow();

    units::VolumeCubicMeters current_water() const { return current_water_; }
    units::VolumeCubicMeters max_capacity() const { return max_capacity_; }
    
    MassBalanceResult get_last_balance() const { return last_balance_; }
    void reset_balance();

private:
    units::VolumeCubicMeters max_capacity_{};
    units::VolumeCubicMeters current_water_{};
    MassBalanceResult last_balance_{};
};

// Conversion utilities
double moles_water_to_cubic_meters(double moles);
double cubic_meters_water_to_moles(double cubic_meters);

} // namespace cannaville::rootzone
