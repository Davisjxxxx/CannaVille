#pragma once

#include "Simulation/Core/Units.hpp"
#include <string>
#include <optional>
#include <vector>

namespace cannaville::rootzone {

enum class WaterStatus {
    Adequate,
    InsufficientRootzoneWater,
    Overflow
};

struct MassBalanceResult {
    double initial_water_volume_m3{0.0};
    double final_water_volume_m3{0.0};
    
    // External Inputs
    double irrigation_added_m3{0.0};
    double top_off_added_m3{0.0};
    double external_return_flow_added_m3{0.0};
    
    // Plant withdrawal
    double plant_withdrawal_requested_m3{0.0};
    double plant_withdrawal_realized_m3{0.0};
    double plant_withdrawal_unmet_m3{0.0};
    
    // External Outputs
    double drainage_removed_m3{0.0};
    double discharge_removed_m3{0.0};
    double evaporation_removed_m3{0.0};
    
    // Ledger
    double residual_m3{0.0}; // Should be very close to 0
    WaterStatus status{WaterStatus::Adequate};
};

struct TranspirationRequest {
    units::WaterFluxMolesPerSquareMeterSecond flux;
    units::AreaSquareMeters area;
};

class SubstrateContainer {
public:
    SubstrateContainer(
        units::VolumeCubicMeters substrate_bulk_volume, 
        units::VolumeCubicMeters initial_water,
        std::optional<units::VolumeCubicMeters> max_stored_water = std::nullopt);

    void irrigate(units::VolumeCubicMeters amount);
    void add_top_off(units::VolumeCubicMeters amount);
    void add_external_return_flow(units::VolumeCubicMeters amount);
    void evaporate(units::VolumeCubicMeters amount);
    
    // Single plant withdrawal. Uses QUASI_STEADY_ROOT_UPTAKE_APPROXIMATION.
    units::VolumeCubicMeters withdraw_transpiration(
        units::WaterFluxMolesPerSquareMeterSecond transpiration_flux,
        units::AreaSquareMeters effective_transpiring_leaf_area,
        units::Seconds delta_time);

    // Shared root zone allocation (proportional shortage policy)
    std::vector<units::VolumeCubicMeters> withdraw_transpiration_shared(
        const std::vector<TranspirationRequest>& requests,
        units::Seconds delta_time);

    // Process overflow if capacity exceeded, or drain explicit amount
    void process_drainage(std::optional<units::VolumeCubicMeters> explicit_drainage = std::nullopt);

    units::VolumeCubicMeters current_water() const { return current_water_; }
    units::VolumeCubicMeters substrate_bulk_volume() const { return substrate_bulk_volume_; }
    std::optional<units::VolumeCubicMeters> max_stored_water() const { return max_stored_water_; }
    
    // Physical Volumetric Water Content calculation
    // VWC = liquid_water_volume_m3 / substrate_bulk_volume_m3
    double volumetric_water_content() const;
    
    // Storage fraction
    // storage_fraction = current_water_volume_m3 / max_stored_water_volume_m3
    std::optional<double> storage_fraction() const;
    
    MassBalanceResult get_last_balance() const { return last_balance_; }
    void reset_balance();

private:
    units::VolumeCubicMeters substrate_bulk_volume_{};
    std::optional<units::VolumeCubicMeters> max_stored_water_{};
    units::VolumeCubicMeters current_water_{};
    MassBalanceResult last_balance_{};
    
    void update_residual();
};

class HydroponicReservoir {
public:
    // Reservoir represents the full recirculating root-zone system
    HydroponicReservoir(
        units::VolumeCubicMeters max_stored_water, 
        units::VolumeCubicMeters initial_water);

    void add_top_off(units::VolumeCubicMeters amount);
    void add_external_return_flow(units::VolumeCubicMeters amount);
    void evaporate(units::VolumeCubicMeters amount);
    
    units::VolumeCubicMeters withdraw_transpiration(
        units::WaterFluxMolesPerSquareMeterSecond transpiration_flux,
        units::AreaSquareMeters effective_transpiring_leaf_area,
        units::Seconds delta_time);

    std::vector<units::VolumeCubicMeters> withdraw_transpiration_shared(
        const std::vector<TranspirationRequest>& requests,
        units::Seconds delta_time);

    void process_discharge(std::optional<units::VolumeCubicMeters> explicit_discharge = std::nullopt);

    units::VolumeCubicMeters current_water() const { return current_water_; }
    units::VolumeCubicMeters max_stored_water() const { return max_stored_water_; }
    
    double storage_fraction() const;
    
    MassBalanceResult get_last_balance() const { return last_balance_; }
    void reset_balance();

private:
    units::VolumeCubicMeters max_stored_water_{};
    units::VolumeCubicMeters current_water_{};
    MassBalanceResult last_balance_{};
    
    void update_residual();
};

// Conversion utilities
double moles_water_to_cubic_meters(double moles);
double cubic_meters_water_to_moles(double cubic_meters);

} // namespace cannaville::rootzone
