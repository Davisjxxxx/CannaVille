#pragma once

#include "Simulation/Core/Units.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace cannaville::core {

struct ScenarioRoomDefinition {
    std::string id;
    units::Meters width;
    units::Meters depth;
    units::Meters cell_size;
};

struct ScenarioPlantDefinition {
    std::string id;
    std::string room_id;
    std::string cultivar_id;
    units::Meters x;
    units::Meters y;
    units::Meters z;
};

struct Scenario {
    std::int64_t schema_version{1};
    std::string scenario_id;
    std::string simulation_version;
    units::Seconds fixed_timestep;
    std::vector<ScenarioRoomDefinition> rooms;
    std::vector<ScenarioPlantDefinition> plants;

    static Scenario load_json(std::string_view json_text);
    std::vector<std::string> validate() const;
};

} // namespace cannaville::core
