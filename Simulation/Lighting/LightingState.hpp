#pragma once

#include "Simulation/Core/Units.hpp"

#include <vector>

namespace cannaville::lighting {

struct LightingScheduleSegment {
    units::Seconds start_of_day;
    units::Seconds end_of_day;
    units::PPFDMicromolesPerSquareMeterSecond ppfd;
};

struct LightingSchedule {
    std::vector<LightingScheduleSegment> segments;
};

struct LightingState {
    units::PPFDMicromolesPerSquareMeterSecond ppfd{};
    units::DLIMolesPerSquareMeterPerDay dli{};
    units::Seconds photoperiod_duration{};
    units::Seconds accumulated_light_on_duration{};
    units::Seconds accumulated_dark_duration{};
    bool light_on{false};
};

} // namespace cannaville::lighting
