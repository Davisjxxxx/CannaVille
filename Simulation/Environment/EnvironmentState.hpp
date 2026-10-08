#pragma once

#include "Simulation/Core/Units.hpp"

namespace cannaville::environment {

struct EnvironmentState {
    units::Celsius air_temperature{};
    units::RelativeHumidityFraction relative_humidity{};
    units::CO2MicromolesPerMole carbon_dioxide{};
    units::AirflowMetersPerSecond airflow{};
    units::Meters cell_height{};
};

} // namespace cannaville::environment
