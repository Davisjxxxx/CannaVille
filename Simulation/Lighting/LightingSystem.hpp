#pragma once

#include "Simulation/Lighting/LightingState.hpp"

#include <string>
#include <vector>

namespace cannaville::lighting {

constexpr double kSimulationDaySeconds = 86400.0;

std::vector<std::string> validate_schedule(const LightingSchedule& schedule);
void initialize_state(LightingState& state, const LightingSchedule& schedule, double simulation_seconds);
void advance_state(LightingState& state,
                   const LightingSchedule& schedule,
                   double simulation_seconds,
                   double duration_seconds);

} // namespace cannaville::lighting
