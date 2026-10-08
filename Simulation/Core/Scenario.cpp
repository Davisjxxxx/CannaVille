#include "Simulation/Core/Scenario.hpp"

#include "Simulation/Core/Json.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace cannaville::core {

namespace {

void require_object(const json::Value& value, const std::string& name) {
    if (!value.is_object()) {
        throw json::ParseError(name + " must be an object");
    }
}

std::string require_string(const json::Value& object, std::string_view key) {
    const json::Value& value = object.require(key);
    if (!value.is_string() || value.as_string().empty()) {
        throw json::ParseError("field " + std::string(key) + " must be a non-empty string");
    }
    return value.as_string();
}

double require_number(const json::Value& object, std::string_view key) {
    const json::Value& value = object.require(key);
    if (!value.is_number() || !std::isfinite(value.as_number())) {
        throw json::ParseError("field " + std::string(key) + " must be a finite number");
    }
    return value.as_number();
}

std::int64_t require_integer(const json::Value& object, std::string_view key) {
    const double value = require_number(object, key);
    if (std::floor(value) != value) {
        throw json::ParseError("field " + std::string(key) + " must be an integer");
    }
    return static_cast<std::int64_t>(value);
}

} // namespace

Scenario Scenario::load_json(std::string_view json_text) {
    const json::Value parsed = json::parse(json_text);
    require_object(parsed, "scenario root");
    const json::Value& root = parsed;
    Scenario scenario;
    scenario.schema_version = require_integer(root, "schema_version");
    scenario.scenario_id = require_string(root, "scenario_id");
    scenario.simulation_version = require_string(root, "simulation_version");
    scenario.fixed_timestep.value = require_number(root, "fixed_timestep_seconds");

    const json::Value& rooms = root.require("rooms");
    if (!rooms.is_array()) {
        throw json::ParseError("rooms must be an array");
    }
    for (const json::Value& room_value : rooms.as_array()) {
        require_object(room_value, "room");
        const json::Value& room = room_value;
        ScenarioRoomDefinition definition;
        definition.id = require_string(room, "id");
        definition.width.value = require_number(room, "width_m");
        definition.depth.value = require_number(room, "depth_m");
        definition.cell_size.value = require_number(room, "cell_size_m");
        scenario.rooms.push_back(std::move(definition));
    }

    const json::Value& plants = root.require("plants");
    if (!plants.is_array()) {
        throw json::ParseError("plants must be an array");
    }
    for (const json::Value& plant_value : plants.as_array()) {
        require_object(plant_value, "plant");
        const json::Value& plant = plant_value;
        ScenarioPlantDefinition definition;
        definition.id = require_string(plant, "id");
        definition.room_id = require_string(plant, "room_id");
        definition.cultivar_id = require_string(plant, "cultivar_id");
        definition.x.value = require_number(plant, "x_m");
        definition.y.value = require_number(plant, "y_m");
        definition.z.value = require_number(plant, "z_m");
        scenario.plants.push_back(std::move(definition));
    }

    const std::vector<std::string> errors = scenario.validate();
    if (!errors.empty()) {
        std::ostringstream message;
        message << "invalid scenario";
        for (const std::string& error : errors) {
            message << "; " << error;
        }
        throw json::ParseError(message.str());
    }
    return scenario;
}

std::vector<std::string> Scenario::validate() const {
    std::vector<std::string> errors;
    if (schema_version != 1) {
        errors.push_back("schema_version must be 1");
    }
    if (scenario_id.empty()) errors.push_back("scenario_id must not be empty");
    if (simulation_version.empty()) errors.push_back("simulation_version must not be empty");
    if (!std::isfinite(fixed_timestep.value) || fixed_timestep.value <= 0.0) {
        errors.push_back("fixed_timestep_seconds must be finite and greater than zero");
    }
    if (fixed_timestep.value > 86400.0) {
        errors.push_back("fixed_timestep_seconds must not exceed one day in the bootstrap");
    }

    std::set<std::string> room_ids;
    for (const ScenarioRoomDefinition& room : rooms) {
        if (room.id.empty()) errors.push_back("room id must not be empty");
        if (!room_ids.insert(room.id).second) errors.push_back("duplicate room id: " + room.id);
        if (!std::isfinite(room.width.value) || room.width.value <= 0.0) {
            errors.push_back("room " + room.id + " width_m must be greater than zero");
        }
        if (!std::isfinite(room.depth.value) || room.depth.value <= 0.0) {
            errors.push_back("room " + room.id + " depth_m must be greater than zero");
        }
        if (!std::isfinite(room.cell_size.value) || room.cell_size.value <= 0.0) {
            errors.push_back("room " + room.id + " cell_size_m must be greater than zero");
        }
        if (room.cell_size.value > room.width.value || room.cell_size.value > room.depth.value) {
            errors.push_back("room " + room.id + " cell_size_m must fit within width_m and depth_m");
        }
        if (room.cell_size.value > 0.0 && room.width.value > 0.0 && room.depth.value > 0.0) {
            const double cell_count = std::ceil(room.width.value / room.cell_size.value) *
                std::ceil(room.depth.value / room.cell_size.value);
            if (cell_count > 10000.0) errors.push_back("room " + room.id + " exceeds bootstrap cell limit");
        }
    }

    std::set<std::string> plant_ids;
    for (const ScenarioPlantDefinition& plant : plants) {
        if (plant.id.empty()) errors.push_back("plant id must not be empty");
        if (!plant_ids.insert(plant.id).second) errors.push_back("duplicate plant id: " + plant.id);
        const auto room_it = std::find_if(rooms.begin(), rooms.end(), [&](const auto& room) {
            return room.id == plant.room_id;
        });
        if (room_it == rooms.end()) {
            errors.push_back("plant " + plant.id + " references unknown room: " + plant.room_id);
            continue;
        }
        if (!std::isfinite(plant.x.value) || plant.x.value < 0.0 || plant.x.value > room_it->width.value) {
            errors.push_back("plant " + plant.id + " x_m is outside its room");
        }
        if (!std::isfinite(plant.y.value) || plant.y.value < 0.0 || plant.y.value > room_it->depth.value) {
            errors.push_back("plant " + plant.id + " y_m is outside its room");
        }
        if (!std::isfinite(plant.z.value) || plant.z.value < 0.0) {
            errors.push_back("plant " + plant.id + " z_m must not be negative");
        }
    }
    return errors;
}

} // namespace cannaville::core
