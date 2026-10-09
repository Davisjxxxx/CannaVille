#pragma once

#include "Simulation/Core/Units.hpp"
#include "Simulation/Environment/EnvironmentState.hpp"
#include "Simulation/Lighting/LightingState.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace cannaville::core {

struct ScenarioCellDefinition {
    std::string id;
    units::Meters center_x;
    units::Meters center_y;
    environment::EnvironmentState environment;
    lighting::LightingSchedule lighting_schedule;
};

struct ScenarioRoomDefinition {
    std::string id;
    units::Meters width;
    units::Meters depth;
    units::Meters cell_size;
    std::vector<ScenarioCellDefinition> cells;
};

struct ScenarioRootZoneDefinition {
    std::string id;
    std::string type; // "Substrate" or "Reservoir"
    units::VolumeCubicMeters substrate_bulk_volume{};
    std::optional<units::VolumeCubicMeters> max_stored_water;
    units::VolumeCubicMeters initial_water_volume{};
};

struct ScenarioPlantDefinition {
    std::string id;
    std::string room_id;
    std::string cultivar_id;
    std::string root_zone_id;
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
    std::vector<ScenarioRootZoneDefinition> root_zones;
    std::vector<ScenarioPlantDefinition> plants;

    static Scenario load_json(std::string_view json_text);
    std::vector<std::string> validate() const;
};

} // namespace cannaville::core
