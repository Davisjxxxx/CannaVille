#pragma once

#include <string>

namespace cannaville::genetics {

struct GeneticsState {
    std::string cultivar_id{"placeholder-cultivar"};
    std::string genotype_reference{"unmodeled"};
    bool inheritance_model_initialized{false};
};

} // namespace cannaville::genetics
