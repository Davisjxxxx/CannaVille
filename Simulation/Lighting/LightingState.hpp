#pragma once

#include "Simulation/Core/Units.hpp"

#include <vector>
#include <optional>

namespace cannaville::lighting {

struct LightingScheduleSegment {
    units::Seconds start_of_day;
    units::Seconds end_of_day;
    units::PPFDMicromolesPerSquareMeterSecond ppfd;
    std::optional<double> incident_shortwave_w_m2;
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
    std::optional<double> incident_shortwave_w_m2;
};

} // namespace cannaville::lighting
