#include "Simulation/Core/Serialization.hpp"

#include "Simulation/Core/Json.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace cannaville::core {

namespace {

json::Value number(double value) { return json::Value(value); }
json::Value string_value(const std::string& value) { return json::Value(value); }
json::Value unsigned_value(std::uint64_t value) { return json::Value(std::to_string(value)); }

double number_field(const json::Value& object, std::string_view key) {
    const json::Value& value = object.require(key);
    if (!value.is_number() || !std::isfinite(value.as_number())) {
        throw json::ParseError("state field " + std::string(key) + " must be a finite number");
    }
    return value.as_number();
}

std::uint64_t unsigned_field(const json::Value& object, std::string_view key) {
    const json::Value& field = object.require(key);
    if (field.is_string()) {
        try {
            std::size_t consumed = 0;
            const std::uint64_t value = std::stoull(field.as_string(), &consumed);
            if (consumed != field.as_string().size()) throw std::invalid_argument("trailing characters");
            return value;
        } catch (const std::exception&) {
            throw json::ParseError("state field " + std::string(key) + " must be a non-negative integer string");
        }
    }
    const double numeric_value = number_field(object, key);
    if (numeric_value < 0.0 || std::floor(numeric_value) != numeric_value ||
        numeric_value > static_cast<double>(std::numeric_limits<std::uint64_t>::max())) {
        throw json::ParseError("state field " + std::string(key) + " must be a non-negative integer");
    }
    return static_cast<std::uint64_t>(numeric_value);
}

std::string string_field(const json::Value& object, std::string_view key) {
    const json::Value& value = object.require(key);
    if (!value.is_string()) throw json::ParseError("state field " + std::string(key) + " must be a string");
    return value.as_string();
}

bool bool_field(const json::Value& object, std::string_view key) {
    const json::Value& value = object.require(key);
    if (!value.is_boolean()) throw json::ParseError("state field " + std::string(key) + " must be boolean");
    return value.as_boolean();
}

std::string stage_to_string(plants::GrowthStage stage) {
    switch (stage) {
    case plants::GrowthStage::Seedling: return "seedling";
    case plants::GrowthStage::Vegetative: return "vegetative";
    case plants::GrowthStage::Flowering: return "flowering";
    case plants::GrowthStage::Harvest: return "harvest";
    }
    throw std::logic_error("unknown growth stage");
}

plants::GrowthStage stage_from_string(const std::string& stage) {
    if (stage == "seedling") return plants::GrowthStage::Seedling;
    if (stage == "vegetative") return plants::GrowthStage::Vegetative;
    if (stage == "flowering") return plants::GrowthStage::Flowering;
    if (stage == "harvest") return plants::GrowthStage::Harvest;
    throw json::ParseError("unknown growth stage: " + stage);
}

json::Value environment_json(const environment::EnvironmentState& value) {
    return json::Value({
        {"air_temperature_c", number(value.air_temperature.value)},
        {"relative_humidity_fraction", number(value.relative_humidity.value)},
        {"co2_umol_per_mol", number(value.carbon_dioxide.value)},
        {"airflow_m_per_s", number(value.airflow.value)},
        {"cell_height_m", number(value.cell_height.value)},
    });
}

void read_environment(const json::Value& object, environment::EnvironmentState& value) {
    value.air_temperature.value = number_field(object, "air_temperature_c");
    value.relative_humidity.value = number_field(object, "relative_humidity_fraction");
    value.carbon_dioxide.value = number_field(object, "co2_umol_per_mol");
    value.airflow.value = number_field(object, "airflow_m_per_s");
    value.cell_height.value = number_field(object, "cell_height_m");
}

json::Value lighting_json(const lighting::LightingState& value) {
    return json::Value({
        {"ppfd_umol_per_m2_s", number(value.ppfd.value)},
        {"dli_mol_per_m2_day", number(value.dli.value)},
        {"photoperiod_hours", number(value.photoperiod.value)},
        {"light_on", json::Value(value.light_on)},
    });
}

void read_lighting(const json::Value& object, lighting::LightingState& value) {
    value.ppfd.value = number_field(object, "ppfd_umol_per_m2_s");
    value.dli.value = number_field(object, "dli_mol_per_m2_day");
    value.photoperiod.value = number_field(object, "photoperiod_hours");
    value.light_on = bool_field(object, "light_on");
}

json::Value root_zone_json(const rootzone::RootZoneState& value) {
    return json::Value({
        {"substrate_moisture_fraction", number(value.substrate_moisture.value)},
        {"root_zone_temperature_c", number(value.root_zone_temperature.value)},
        {"ph", number(value.ph.value)},
        {"ec_ms_per_cm", number(value.electrical_conductivity.value)},
    });
}

void read_root_zone(const json::Value& object, rootzone::RootZoneState& value) {
    value.substrate_moisture.value = number_field(object, "substrate_moisture_fraction");
    value.root_zone_temperature.value = number_field(object, "root_zone_temperature_c");
    value.ph.value = number_field(object, "ph");
    value.electrical_conductivity.value = number_field(object, "ec_ms_per_cm");
}

json::Value plant_json(const plants::PlantState& plant) {
    return json::Value({
        {"id", string_value(plant.id)},
        {"location", json::Value({
            {"room_id", string_value(plant.location.room_id)},
            {"x_m", number(plant.location.x.value)},
            {"y_m", number(plant.location.y.value)},
            {"z_m", number(plant.location.z.value)},
        })},
        {"genetics", json::Value({
            {"cultivar_id", string_value(plant.genetics.cultivar_id)},
            {"genotype_reference", string_value(plant.genetics.genotype_reference)},
            {"inheritance_model_initialized", json::Value(plant.genetics.inheritance_model_initialized)},
        })},
        {"sampled_environment", environment_json(plant.sampled_environment)},
        {"sampled_lighting", lighting_json(plant.sampled_lighting)},
        {"root_zone", root_zone_json(plant.root_zone)},
        {"nutrition", json::Value({
            {"solution_profile_id", string_value(plant.nutrition.solution_profile_id)},
            {"availability_model_initialized", json::Value(plant.nutrition.availability_model_initialized)},
            {"uptake_model_initialized", json::Value(plant.nutrition.uptake_model_initialized)},
        })},
        {"pests", json::Value({
            {"population_model_id", string_value(plant.pests.population_model_id)},
            {"life_cycle_model_initialized", json::Value(plant.pests.life_cycle_model_initialized)},
        })},
        {"disease", json::Value({
            {"pathogen_model_id", string_value(plant.disease.pathogen_model_id)},
            {"latent_infection_model_initialized", json::Value(plant.disease.latent_infection_model_initialized)},
        })},
        {"treatment", json::Value({
            {"active_treatment_id", string_value(plant.treatment.active_treatment_id)},
            {"efficacy_model_initialized", json::Value(plant.treatment.efficacy_model_initialized)},
            {"coverage_model_initialized", json::Value(plant.treatment.coverage_model_initialized)},
        })},
        {"harvest", json::Value({
            {"harvestable", json::Value(plant.harvest.harvestable)},
            {"wet_mass_g", number(plant.harvest.wet_mass.value)},
            {"dry_mass_g", number(plant.harvest.dry_mass.value)},
        })},
        {"drying", json::Value({
            {"active", json::Value(plant.drying.active)},
            {"elapsed_s", number(plant.drying.elapsed.value)},
            {"product_moisture_fraction", number(plant.drying.product_moisture.value)},
        })},
        {"curing", json::Value({
            {"active", json::Value(plant.curing.active)},
            {"elapsed_s", number(plant.curing.elapsed.value)},
            {"quality_model_initialized", json::Value(plant.curing.quality_model_initialized)},
        })},
        {"latent", json::Value(json::Value::Object{
            {"growth_stage", string_value(stage_to_string(plant.latent.growth_stage))},
        })},
        {"observable", json::Value({
            {"displayed_growth_stage", string_value(stage_to_string(plant.observable.displayed_growth_stage))},
            {"interaction_available", json::Value(plant.observable.interaction_available)},
        })},
        {"derived", json::Value(json::Value::Object{
            {"readiness_label", string_value(plant.derived.readiness_label)},
        })},
        {"history", json::Value({
            {"elapsed_s", number(plant.history.elapsed.value)},
            {"steps", unsigned_value(plant.history.steps.value)},
        })},
        {"stochastic", json::Value({
            {"stream_seed", unsigned_value(plant.stochastic.stream_seed)},
            {"stream_state", unsigned_value(plant.stochastic.stream_state)},
            {"draws_consumed", unsigned_value(plant.stochastic.draws_consumed)},
        })},
    });
}

void read_plant(const json::Value& object, plants::PlantState& plant) {
    plant.id = string_field(object, "id");
    const auto& location = object.require("location");
    plant.location.room_id = string_field(location, "room_id");
    plant.location.x.value = number_field(location, "x_m");
    plant.location.y.value = number_field(location, "y_m");
    plant.location.z.value = number_field(location, "z_m");

    const auto& genetics = object.require("genetics");
    plant.genetics.cultivar_id = string_field(genetics, "cultivar_id");
    plant.genetics.genotype_reference = string_field(genetics, "genotype_reference");
    plant.genetics.inheritance_model_initialized = bool_field(genetics, "inheritance_model_initialized");
    read_environment(object.require("sampled_environment"), plant.sampled_environment);
    read_lighting(object.require("sampled_lighting"), plant.sampled_lighting);
    read_root_zone(object.require("root_zone"), plant.root_zone);

    const auto& nutrition = object.require("nutrition");
    plant.nutrition.solution_profile_id = string_field(nutrition, "solution_profile_id");
    plant.nutrition.availability_model_initialized = bool_field(nutrition, "availability_model_initialized");
    plant.nutrition.uptake_model_initialized = bool_field(nutrition, "uptake_model_initialized");
    const auto& pests = object.require("pests");
    plant.pests.population_model_id = string_field(pests, "population_model_id");
    plant.pests.life_cycle_model_initialized = bool_field(pests, "life_cycle_model_initialized");
    const auto& disease = object.require("disease");
    plant.disease.pathogen_model_id = string_field(disease, "pathogen_model_id");
    plant.disease.latent_infection_model_initialized = bool_field(disease, "latent_infection_model_initialized");
    const auto& treatment = object.require("treatment");
    plant.treatment.active_treatment_id = string_field(treatment, "active_treatment_id");
    plant.treatment.efficacy_model_initialized = bool_field(treatment, "efficacy_model_initialized");
    plant.treatment.coverage_model_initialized = bool_field(treatment, "coverage_model_initialized");
    const auto& harvest = object.require("harvest");
    plant.harvest.harvestable = bool_field(harvest, "harvestable");
    plant.harvest.wet_mass.value = number_field(harvest, "wet_mass_g");
    plant.harvest.dry_mass.value = number_field(harvest, "dry_mass_g");
    const auto& drying = object.require("drying");
    plant.drying.active = bool_field(drying, "active");
    plant.drying.elapsed.value = number_field(drying, "elapsed_s");
    plant.drying.product_moisture.value = number_field(drying, "product_moisture_fraction");
    const auto& curing = object.require("curing");
    plant.curing.active = bool_field(curing, "active");
    plant.curing.elapsed.value = number_field(curing, "elapsed_s");
    plant.curing.quality_model_initialized = bool_field(curing, "quality_model_initialized");
    plant.latent.growth_stage = stage_from_string(string_field(object.require("latent"), "growth_stage"));
    const auto& observable = object.require("observable");
    plant.observable.displayed_growth_stage = stage_from_string(string_field(observable, "displayed_growth_stage"));
    plant.observable.interaction_available = bool_field(observable, "interaction_available");
    plant.derived.readiness_label = string_field(object.require("derived"), "readiness_label");
    const auto& history = object.require("history");
    plant.history.elapsed.value = number_field(history, "elapsed_s");
    plant.history.steps.value = unsigned_field(history, "steps");
    const auto& stochastic = object.require("stochastic");
    plant.stochastic.stream_seed = unsigned_field(stochastic, "stream_seed");
    plant.stochastic.stream_state = unsigned_field(stochastic, "stream_state");
    plant.stochastic.draws_consumed = unsigned_field(stochastic, "draws_consumed");
}

json::Value cell_json(const RoomCellState& cell) {
    return json::Value({
        {"id", string_value(cell.id)},
        {"center_x_m", number(cell.center_x.value)},
        {"center_y_m", number(cell.center_y.value)},
        {"environment", environment_json(cell.environment)},
        {"lighting", lighting_json(cell.lighting)},
    });
}

void read_cell(const json::Value& object, RoomCellState& cell) {
    cell.id = string_field(object, "id");
    cell.center_x.value = number_field(object, "center_x_m");
    cell.center_y.value = number_field(object, "center_y_m");
    read_environment(object.require("environment"), cell.environment);
    read_lighting(object.require("lighting"), cell.lighting);
}

json::Value room_json(const RoomState& room) {
    json::Value::Array cells;
    for (const RoomCellState& cell : room.cells) cells.push_back(cell_json(cell));
    return json::Value({
        {"id", string_value(room.id)},
        {"width_m", number(room.width.value)},
        {"depth_m", number(room.depth.value)},
        {"cell_size_m", number(room.cell_size.value)},
        {"cells", json::Value(std::move(cells))},
    });
}

void read_room(const json::Value& object, RoomState& room) {
    room.id = string_field(object, "id");
    room.width.value = number_field(object, "width_m");
    room.depth.value = number_field(object, "depth_m");
    room.cell_size.value = number_field(object, "cell_size_m");
    const auto& cells = object.require("cells");
    if (!cells.is_array()) throw json::ParseError("state cells must be an array");
    for (const auto& value : cells.as_array()) {
        RoomCellState cell;
        read_cell(value, cell);
        room.cells.push_back(std::move(cell));
    }
}

} // namespace

std::string serialize_state_json(const SimulationState& state) {
    json::Value::Array rooms;
    for (const RoomState& room : state.rooms) rooms.push_back(room_json(room));
    json::Value::Array plant_values;
    for (const cannaville::plants::PlantState& plant : state.plants) plant_values.push_back(plant_json(plant));

    return json::stringify(json::Value({
        {"state_schema_version", number(1.0)},
        {"config", json::Value({
            {"simulation_version", string_value(state.config.simulation_version)},
            {"fixed_timestep_seconds", number(state.config.fixed_timestep.value)},
        })},
        {"scenario_id", string_value(state.scenario_id)},
        {"clock", json::Value({
            {"elapsed_seconds", number(state.clock.elapsed.value)},
            {"steps", unsigned_value(state.clock.steps.value)},
        })},
        {"stochastic", json::Value({
            {"seed", unsigned_value(state.stochastic.seed)},
            {"rng_state", unsigned_value(state.stochastic.rng_state)},
            {"draws_consumed", unsigned_value(state.stochastic.draws_consumed)},
        })},
        {"rooms", json::Value(std::move(rooms))},
        {"plants", json::Value(std::move(plant_values))},
    }));
}

SimulationState deserialize_state_json(const std::string& serialized) {
    const json::Value root = json::parse(serialized);
    if (!root.is_object()) throw json::ParseError("serialized state root must be an object");
    if (number_field(root, "state_schema_version") != 1.0) {
        throw json::ParseError("unsupported state_schema_version");
    }
    SimulationState state;
    const auto& config = root.require("config");
    state.config.simulation_version = string_field(config, "simulation_version");
    state.config.fixed_timestep.value = number_field(config, "fixed_timestep_seconds");
    state.scenario_id = string_field(root, "scenario_id");
    const auto& clock = root.require("clock");
    state.clock.elapsed.value = number_field(clock, "elapsed_seconds");
    state.clock.steps.value = unsigned_field(clock, "steps");
    const auto& stochastic = root.require("stochastic");
    state.stochastic.seed = unsigned_field(stochastic, "seed");
    state.stochastic.rng_state = unsigned_field(stochastic, "rng_state");
    state.stochastic.draws_consumed = unsigned_field(stochastic, "draws_consumed");
    const auto& rooms = root.require("rooms");
    if (!rooms.is_array()) throw json::ParseError("serialized rooms must be an array");
    for (const auto& value : rooms.as_array()) {
        RoomState room;
        read_room(value, room);
        state.rooms.push_back(std::move(room));
    }
    const auto& plants = root.require("plants");
    if (!plants.is_array()) throw json::ParseError("serialized plants must be an array");
    for (const auto& value : plants.as_array()) {
        plants::PlantState plant;
        read_plant(value, plant);
        state.plants.push_back(std::move(plant));
    }
    return state;
}

} // namespace cannaville::core
