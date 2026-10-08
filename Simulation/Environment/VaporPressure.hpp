#pragma once

#include "Simulation/Core/Units.hpp"

#include <optional>

namespace cannaville::environment {

struct EnvironmentState;

constexpr double kFao56MinimumTemperatureCelsius = 0.0;
constexpr double kFao56MaximumTemperatureCelsius = 50.0;

units::VaporPressureKPa saturation_vapor_pressure_fao56(units::Celsius temperature);
units::VaporPressureKPa actual_vapor_pressure_from_relative_humidity(
    units::VaporPressureKPa saturation_vapor_pressure,
    units::RelativeHumidityPercent relative_humidity);
units::VPDKPa air_vpd(units::VaporPressureKPa saturation_vapor_pressure,
                      units::VaporPressureKPa actual_vapor_pressure);
std::optional<units::VPDKPa> leaf_vpd(
    std::optional<units::Celsius> leaf_temperature,
    units::VaporPressureKPa ambient_actual_vapor_pressure);

void derive_physical_state(EnvironmentState& state);

} // namespace cannaville::environment
