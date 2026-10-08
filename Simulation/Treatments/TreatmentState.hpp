#pragma once

#include <string>

namespace cannaville::treatments {

struct TreatmentState {
    std::string active_treatment_id{"none"};
    bool efficacy_model_initialized{false};
    bool coverage_model_initialized{false};
};

} // namespace cannaville::treatments
