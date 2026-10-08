#pragma once

#include "Simulation/Core/Units.hpp"

namespace cannaville::lighting {

struct LightingState {
    units::PPFDMicromolesPerSquareMeterSecond ppfd{};
    units::DLIMolesPerSquareMeterPerDay dli{};
    units::PhotoperiodHours photoperiod{};
    bool light_on{false};
};

} // namespace cannaville::lighting
