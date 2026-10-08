#pragma once

#include "Simulation/Core/Units.hpp"

namespace cannaville::rootzone {

struct RootZoneState {
    units::SubstrateMoistureFraction substrate_moisture{};
    units::TemperatureCelsius root_zone_temperature{};
    units::PH ph{};
    units::EcmilliSiemensPerCentimeter electrical_conductivity{};
};

} // namespace cannaville::rootzone
