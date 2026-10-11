#include "Simulation/Core/Scenario.hpp"

#include "Simulation/Core/Json.hpp"
#include "Simulation/Environment/VaporPressure.hpp"
#include "Simulation/Lighting/LightingSystem.hpp"

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

environment::EnvironmentState parse_environment(const json::Value& object) {
    require_object(object, "environment");
    environment::EnvironmentState state;
    state.air_temperature.value = require_number(object, "air_temperature_c");
    state.relative_humidity.value = require_number(object, "relative_humidity_percent");
    state.atmospheric_pressure.value = require_number(object, "atmospheric_pressure_kpa");
    state.carbon_dioxide.value = require_number(object, "co2_umol_per_mol");
    const json::Value* airflow = object.find("airflow_m_per_s");
    if (airflow != nullptr && !airflow->is_null()) {
        state.airflow = units::AirflowMetersPerSecond{airflow->as_number()};
    }
    const json::Value* leaf_temperature = object.find("leaf_temperature_c");
    if (leaf_temperature != nullptr && !leaf_temperature->is_null()) {
        if (!leaf_temperature->is_number() || !std::isfinite(leaf_temperature->as_number())) {
            throw json::ParseError("leaf_temperature_c must be a finite number or null");
        }
        state.leaf_temperature = units::Celsius{leaf_temperature->as_number()};
    }
    const json::Value* air_velocity = object.find("air_velocity_m_s");
    if (air_velocity != nullptr && !air_velocity->is_null()) {
        state.airflow = units::AirflowMetersPerSecond{air_velocity->as_number()};
    }
    environment::derive_physical_state(state);
    return state;
}

lighting::LightingSchedule parse_lighting_schedule(const json::Value& object) {
    const json::Value& value = object.require("lighting_schedule");
    if (!value.is_array()) throw json::ParseError("lighting_schedule must be an array");
    lighting::LightingSchedule schedule;
    for (const json::Value& segment_value : value.as_array()) {
        require_object(segment_value, "lighting schedule segment");
        lighting::LightingScheduleSegment segment;
        segment.start_of_day.value = require_number(segment_value, "start_seconds");
        segment.end_of_day.value = require_number(segment_value, "end_seconds");
        segment.ppfd.value = require_number(segment_value, "ppfd_umol_per_m2_s");
        schedule.segments.push_back(segment);
    }
    const std::vector<std::string> errors = lighting::validate_schedule(schedule);
    if (!errors.empty()) throw json::ParseError(errors.front());
    return schedule;
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
        const json::Value* cells = room.find("cells");
        if (cells != nullptr) {
            if (!cells->is_array()) throw json::ParseError("room cells must be an array");
            for (const json::Value& cell_value : cells->as_array()) {
                require_object(cell_value, "cell");
                ScenarioCellDefinition cell;
                cell.id = require_string(cell_value, "id");
                cell.center_x.value = require_number(cell_value, "center_x_m");
                cell.center_y.value = require_number(cell_value, "center_y_m");
                cell.environment = parse_environment(cell_value.require("environment"));
                cell.lighting_schedule = parse_lighting_schedule(cell_value);
                definition.cells.push_back(std::move(cell));
            }
        }
        scenario.rooms.push_back(std::move(definition));
    }

    const json::Value* root_zones = root.find("root_zones");
    if (root_zones != nullptr) {
        if (!root_zones->is_array()) throw json::ParseError("root_zones must be an array");
        for (const json::Value& rz_value : root_zones->as_array()) {
            require_object(rz_value, "root_zone");
            ScenarioRootZoneDefinition rz;
            rz.id = require_string(rz_value, "id");
            rz.type = require_string(rz_value, "type");
            const json::Value* sub_vol = rz_value.find("substrate_bulk_volume_m3");
            if (sub_vol != nullptr) rz.substrate_bulk_volume.value = sub_vol->as_number();
            else rz.substrate_bulk_volume.value = 0.0;
            const json::Value* max_water = rz_value.find("max_stored_water_m3");
            if (max_water != nullptr) {
                rz.max_stored_water = units::VolumeCubicMeters{max_water->as_number()};
            }
            rz.initial_water_volume.value = require_number(rz_value, "initial_water_volume_m3");
            const json::Value* sub_prof = rz_value.find("substrate_hydraulic_profile_id");
            if (sub_prof != nullptr && sub_prof->is_string()) rz.substrate_hydraulic_profile_id = sub_prof->as_string();
            const json::Value* stress_prof = rz_value.find("hydraulic_stress_transfer_profile_id");
            if (stress_prof != nullptr && stress_prof->is_string()) rz.hydraulic_stress_transfer_profile_id = stress_prof->as_string();
            const json::Value* unrestricted = rz_value.find("explicit_unrestricted_water_access");
            if (unrestricted != nullptr && unrestricted->is_boolean()) rz.explicit_unrestricted_water_access = unrestricted->as_boolean();
            scenario.root_zones.push_back(std::move(rz));
        }
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
        const json::Value* rzid = plant.find("root_zone_id");
        if (rzid != nullptr) {
            definition.root_zone_id = rzid->as_string();
        } else {
            definition.root_zone_id = "rootzone_" + definition.id; // Default fallback for backward compatibility
        }
        definition.x.value = require_number(plant, "x_m");
        definition.y.value = require_number(plant, "y_m");
        definition.z.value = require_number(plant, "z_m");
        const json::Value* gep = plant.find("gas_exchange_profile_id");
        if (gep) definition.gas_exchange_profile_id = gep->as_string();
        const json::Value* etl = plant.find("effective_transpiring_leaf_area_m2");
        if (etl) definition.effective_transpiring_leaf_area_m2 = etl->as_number();
        const json::Value* lcd = plant.find("leaf_characteristic_dimension_m");
        if (lcd) definition.leaf_characteristic_dimension_m = lcd->as_number();
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
    const json::Value* water_events = root.find("water_events");
    if (water_events != nullptr) {
        if (!water_events->is_array()) throw json::ParseError("water_events must be an array");
        for (const json::Value& ev_wrapper : water_events->as_array()) {
            require_object(ev_wrapper, "water_event");
            const json::Value& ev_val = ev_wrapper; // assuming no wrapper used originally, if require_object just checks if it's an object
            // Let's actually check how require_object is implemented. It just checks if ev_val is an object!
            ScenarioWaterEvent ev;
            ev.id = require_string(ev_val, "id");
            ev.timestamp.value = require_number(ev_val, "timestamp_s");
            ev.root_zone_id = require_string(ev_val, "root_zone_id");
            ev.type = require_string(ev_val, "type");
            ev.amount.value = require_number(ev_val, "amount_m3");
            scenario.water_events.push_back(std::move(ev));
        }
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
    if (rooms.empty()) errors.push_back("at least one room is required");

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
        std::set<std::string> cell_ids;
        for (const ScenarioCellDefinition& cell : room.cells) {
            if (cell.id.empty()) errors.push_back("room " + room.id + " cell id must not be empty");
            if (!cell_ids.insert(cell.id).second) errors.push_back("duplicate cell id: " + cell.id);
            if (!std::isfinite(cell.center_x.value) || cell.center_x.value < 0.0 ||
                cell.center_x.value > room.width.value) {
                errors.push_back("cell " + cell.id + " center_x_m is outside its room");
            }
            if (!std::isfinite(cell.center_y.value) || cell.center_y.value < 0.0 ||
                cell.center_y.value > room.depth.value) {
                errors.push_back("cell " + cell.id + " center_y_m is outside its room");
            }
            if (!cell.environment.physics_available) {
                errors.push_back("cell " + cell.id + " has no validated physical environment state");
            }
            const auto schedule_errors = lighting::validate_schedule(cell.lighting_schedule);
            errors.insert(errors.end(), schedule_errors.begin(), schedule_errors.end());
        }
    }

    std::set<std::string> root_zone_ids;
    for (const ScenarioRootZoneDefinition& rz : root_zones) {
        if (rz.id.empty()) errors.push_back("root_zone id must not be empty");
        if (!root_zone_ids.insert(rz.id).second) errors.push_back("duplicate root_zone id: " + rz.id);
        if (rz.type != "Substrate" && rz.type != "Reservoir") {
            errors.push_back("root_zone " + rz.id + " type must be Substrate or Reservoir");
        }
        
        if (rz.type == "Substrate") {
            if (rz.substrate_bulk_volume.value <= 0.0) {
                errors.push_back("root_zone " + rz.id + " substrate_bulk_volume_m3 must be positive");
            }
        } else if (rz.type == "Reservoir") {
            if (!rz.max_stored_water.has_value() || rz.max_stored_water->value <= 0.0) {
                errors.push_back("root_zone " + rz.id + " capacity_m3/max_stored_water_m3 must be positive for reservoir");
            }
            if (rz.max_stored_water.has_value() && rz.initial_water_volume.value > rz.max_stored_water->value) {
                errors.push_back("root_zone " + rz.id + " initial_water_volume_m3 cannot exceed capacity");
            }
        }
        
        if (rz.max_stored_water.has_value() && rz.max_stored_water->value < 0.0) {
            errors.push_back("root_zone " + rz.id + " max_stored_water_m3 cannot be negative");
        }
        if (rz.initial_water_volume.value < 0.0) {
            errors.push_back("root_zone " + rz.id + " initial_water_volume_m3 cannot be negative");
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
        if (!root_zones.empty()) {
            if (root_zone_ids.find(plant.root_zone_id) == root_zone_ids.end()) {
                errors.push_back("plant " + plant.id + " references unknown root_zone: " + plant.root_zone_id);
            }
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
    for (const ScenarioWaterEvent& ev : water_events) {
        if (ev.id.empty()) errors.push_back("water_event id must not be empty");
        if (ev.timestamp.value < 0.0) errors.push_back("water_event timestamp cannot be negative");
        if (root_zone_ids.find(ev.root_zone_id) == root_zone_ids.end()) errors.push_back("water_event references unknown root zone: " + ev.root_zone_id);
        if (ev.type != "irrigation" && ev.type != "top_off" && ev.type != "external_return" && ev.type != "drainage" && ev.type != "discharge") {
            errors.push_back("water_event type unsupported: " + ev.type);
        }
    }
    
    return errors;
}


} // namespace cannaville::core
