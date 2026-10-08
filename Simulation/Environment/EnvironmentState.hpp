#pragma once

#include "Simulation/Core/Units.hpp"

#include <optional>

namespace cannaville::environment {

struct EnvironmentState {
    units::Celsius air_temperature{};
    units::RelativeHumidityPercent relative_humidity{};
    units::AtmosphericPressureKPa atmospheric_pressure{};
    units::CO2MicromolesPerMole carbon_dioxide{};
    units::AirflowMetersPerSecond airflow{};
    units::Meters cell_height{};
    std::optional<units::Celsius> leaf_temperature;

    // Derived physical measurements. They are unavailable until the input
    // measurements have passed validation and the registered models run.
    units::VaporPressureKPa saturation_vapor_pressure{};
    units::VaporPressureKPa actual_vapor_pressure{};
    units::VPDKPa air_vpd{};
    std::optional<units::VPDKPa> leaf_vpd;
    bool physics_available{false};
};

} // namespace cannaville::environment
