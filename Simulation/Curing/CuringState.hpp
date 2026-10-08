#pragma once

#include "Simulation/Core/Units.hpp"

namespace cannaville::curing {

struct CuringState {
    bool active{false};
    units::Seconds elapsed{};
    bool quality_model_initialized{false};
};

} // namespace cannaville::curing
