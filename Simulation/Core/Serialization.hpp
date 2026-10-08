#pragma once

#include "Simulation/Core/SimulationState.hpp"

#include <string>

namespace cannaville::core {

// Stable JSON serialization for state snapshots. Object keys are ordered by
// the standard map-backed JSON representation; arrays retain simulation order.
std::string serialize_state_json(const SimulationState& state);
SimulationState deserialize_state_json(const std::string& serialized);

} // namespace cannaville::core
