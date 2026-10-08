#pragma once

#include "Simulation/Core/Units.hpp"

namespace cannaville::drying {

struct DryingState {
    bool active{false};
    units::Seconds elapsed{};
    units::RelativeHumidityFraction product_moisture{};
};

} // namespace cannaville::drying
