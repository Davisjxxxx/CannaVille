#pragma once

#include <string>

namespace cannaville::nutrition {

struct NutritionState {
    // Species and uptake models are intentionally absent in the bootstrap.
    std::string solution_profile_id{"unmodeled"};
    bool availability_model_initialized{false};
    bool uptake_model_initialized{false};
};

} // namespace cannaville::nutrition
