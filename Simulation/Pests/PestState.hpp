#pragma once

#include <string>

namespace cannaville::pests {

struct PestPopulationState {
    std::string population_model_id{"unmodeled"};
    bool life_cycle_model_initialized{false};
};

} // namespace cannaville::pests
