#pragma once

#include <string>

namespace cannaville::disease {

struct DiseaseState {
    std::string pathogen_model_id{"unmodeled"};
    bool latent_infection_model_initialized{false};
};

} // namespace cannaville::disease
