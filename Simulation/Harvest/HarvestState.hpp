#pragma once

#include "Simulation/Core/Units.hpp"

namespace cannaville::harvest {

struct HarvestState {
    bool harvestable{false};
    units::MassGrams wet_mass{};
    units::MassGrams dry_mass{};
};

} // namespace cannaville::harvest
