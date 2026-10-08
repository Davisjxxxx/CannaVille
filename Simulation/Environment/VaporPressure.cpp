#include "Simulation/Environment/VaporPressure.hpp"

#include "Simulation/Environment/EnvironmentState.hpp"

#include <cmath>
#include <stdexcept>

namespace cannaville::environment {

units::VaporPressureKPa saturation_vapor_pressure_fao56(units::Celsius temperature) {
    if (!std::isfinite(temperature.value) || temperature.value < kSupportedMinimumTemperatureCelsius ||
        temperature.value > kSupportedMaximumTemperatureCelsius) {
        throw std::invalid_argument("Implementation supported domain requires 0 <= temperature_c <= 50");
    }
    const double numerator = 17.27 * temperature.value;
    const double denominator = temperature.value + 237.3;
    return units::VaporPressureKPa{0.6108 * std::exp(numerator / denominator)};
}

units::VaporPressureKPa actual_vapor_pressure_from_relative_humidity(
    units::VaporPressureKPa saturation_vapor_pressure,
    units::RelativeHumidityPercent relative_humidity) {
    if (!std::isfinite(saturation_vapor_pressure.value) || saturation_vapor_pressure.value < 0.0) {
        throw std::invalid_argument("saturation vapor pressure must be finite and non-negative");
    }
    if (!std::isfinite(relative_humidity.value) || relative_humidity.value < 0.0 ||
        relative_humidity.value > 100.0) {
        throw std::invalid_argument("relative humidity percent must be within [0, 100]");
    }
    return units::VaporPressureKPa{saturation_vapor_pressure.value * relative_humidity.value / 100.0};
}

units::VPDKPa air_vpd(units::VaporPressureKPa saturation_vapor_pressure,
                      units::VaporPressureKPa actual_vapor_pressure) {
    if (!std::isfinite(saturation_vapor_pressure.value) || !std::isfinite(actual_vapor_pressure.value) ||
        saturation_vapor_pressure.value < 0.0 || actual_vapor_pressure.value < 0.0 ||
        actual_vapor_pressure.value > saturation_vapor_pressure.value + 1e-12) {
        throw std::invalid_argument("vapor pressures are physically inconsistent");
    }
    return units::VPDKPa{saturation_vapor_pressure.value - actual_vapor_pressure.value};
}

std::optional<units::VPDKPa> leaf_vpd(
    std::optional<units::Celsius> leaf_temperature,
    units::VaporPressureKPa ambient_actual_vapor_pressure) {
    if (!leaf_temperature.has_value()) return std::nullopt;
    const units::VaporPressureKPa leaf_saturation = saturation_vapor_pressure_fao56(*leaf_temperature);
    if (!std::isfinite(ambient_actual_vapor_pressure.value) || ambient_actual_vapor_pressure.value < 0.0) {
        throw std::invalid_argument("ambient actual vapor pressure must be finite and non-negative");
    }
    // Negative leaf VPD is retained as a physically meaningful condensation
    // condition rather than being silently clamped.
    return units::VPDKPa{leaf_saturation.value - ambient_actual_vapor_pressure.value};
}

void derive_physical_state(EnvironmentState& state) {
    if (!std::isfinite(state.atmospheric_pressure.value) || state.atmospheric_pressure.value <= 0.0) {
        throw std::invalid_argument("atmospheric pressure must be finite and greater than zero");
    }
    if (!std::isfinite(state.carbon_dioxide.value) || state.carbon_dioxide.value < 0.0) {
        throw std::invalid_argument("CO2 concentration must be finite and non-negative");
    }
    const units::VaporPressureKPa saturation = saturation_vapor_pressure_fao56(state.air_temperature);
    const units::VaporPressureKPa actual = actual_vapor_pressure_from_relative_humidity(
        saturation, state.relative_humidity);
    state.saturation_vapor_pressure = saturation;
    state.actual_vapor_pressure = actual;
    state.air_vpd = air_vpd(saturation, actual);
    state.leaf_vpd = leaf_vpd(state.leaf_temperature, actual);
    state.physics_available = true;
}

} // namespace cannaville::environment
