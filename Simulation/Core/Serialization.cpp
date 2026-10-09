#include "Simulation/Core/Serialization.hpp"

#include "Simulation/Core/Json.hpp"

#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>

namespace cannaville::core {

namespace {

json::Value number(double value) { return json::Value(value); }
json::Value string_value(const std::string& value) { return json::Value(value); }
json::Value unsigned_value(std::uint64_t value) { return json::Value(std::to_string(value)); }
template <typename T>
json::Value optional_number(const std::optional<T>& value) {
    return value.has_value() ? number(value->value) : json::Value(nullptr);
}
json::Value optional_number(const std::optional<double>& value) {
    return value.has_value() ? number(*value) : json::Value(nullptr);
}
json::Value optional_string(const std::optional<std::string>& value) {
    return value.has_value() ? string_value(*value) : json::Value(nullptr);
}
json::Value optional_bool(const std::optional<bool>& value) {
    return value.has_value() ? json::Value(*value) : json::Value(nullptr);
}

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
        {"relative_humidity_percent", number(value.relative_humidity.value)},
        {"atmospheric_pressure_kpa", number(value.atmospheric_pressure.value)},
        {"co2_umol_per_mol", number(value.carbon_dioxide.value)},
        {"airflow_m_per_s", optional_number(value.airflow)},
        {"cell_height_m", number(value.cell_height.value)},
        {"leaf_temperature_c", optional_number(value.leaf_temperature)},
        {"saturation_vapor_pressure_kpa", number(value.saturation_vapor_pressure.value)},
        {"actual_vapor_pressure_kpa", number(value.actual_vapor_pressure.value)},
        {"air_vpd_kpa", number(value.air_vpd.value)},
        {"leaf_vpd_kpa", value.leaf_vpd.has_value() ? number(value.leaf_vpd->value) : json::Value(nullptr)},
        {"physics_available", json::Value(value.physics_available)},
    });
}

void read_environment(const json::Value& object, environment::EnvironmentState& value) {
    value.air_temperature.value = number_field(object, "air_temperature_c");
    value.relative_humidity.value = number_field(object, "relative_humidity_percent");
    value.atmospheric_pressure.value = number_field(object, "atmospheric_pressure_kpa");
    value.carbon_dioxide.value = number_field(object, "co2_umol_per_mol");
    const json::Value& airflow = object.require("airflow_m_per_s");
    if (airflow.is_null()) {
        value.airflow.reset();
    } else {
        value.airflow = units::AirflowMetersPerSecond{airflow.as_number()};
    }
    value.cell_height.value = number_field(object, "cell_height_m");
    const json::Value& leaf_temperature = object.require("leaf_temperature_c");
    if (leaf_temperature.is_null()) {
        value.leaf_temperature.reset();
    } else {
        if (!leaf_temperature.is_number()) throw json::ParseError("leaf_temperature_c must be a number or null");
        value.leaf_temperature = units::Celsius{leaf_temperature.as_number()};
    }
    value.saturation_vapor_pressure.value = number_field(object, "saturation_vapor_pressure_kpa");
    value.actual_vapor_pressure.value = number_field(object, "actual_vapor_pressure_kpa");
    value.air_vpd.value = number_field(object, "air_vpd_kpa");
    const json::Value& leaf_vpd = object.require("leaf_vpd_kpa");
    if (leaf_vpd.is_null()) value.leaf_vpd.reset();
    else value.leaf_vpd = units::VPDKPa{number_field(object, "leaf_vpd_kpa")};
    value.physics_available = bool_field(object, "physics_available");
}

json::Value lighting_json(const lighting::LightingState& value) {
    return json::Value({
        {"ppfd_umol_per_m2_s", number(value.ppfd.value)},
        {"dli_mol_per_m2_day", number(value.dli.value)},
        {"photoperiod_duration_s", number(value.photoperiod_duration.value)},
        {"accumulated_light_on_duration_s", number(value.accumulated_light_on_duration.value)},
        {"accumulated_dark_duration_s", number(value.accumulated_dark_duration.value)},
        {"light_on", json::Value(value.light_on)},
    });
}

void read_lighting(const json::Value& object, lighting::LightingState& value) {
    value.ppfd.value = number_field(object, "ppfd_umol_per_m2_s");
    value.dli.value = number_field(object, "dli_mol_per_m2_day");
    value.photoperiod_duration.value = number_field(object, "photoperiod_duration_s");
    value.accumulated_light_on_duration.value = number_field(object, "accumulated_light_on_duration_s");
    value.accumulated_dark_duration.value = number_field(object, "accumulated_dark_duration_s");
    value.light_on = bool_field(object, "light_on");
}

json::Value schedule_json(const lighting::LightingSchedule& schedule) {
    json::Value::Array segments;
    for (const auto& segment : schedule.segments) {
        segments.push_back(json::Value({
            {"start_seconds", number(segment.start_of_day.value)},
            {"end_seconds", number(segment.end_of_day.value)},
            {"ppfd_umol_per_m2_s", number(segment.ppfd.value)},
        }));
    }
    return json::Value(std::move(segments));
}

lighting::LightingSchedule read_schedule(const json::Value& value) {
    if (!value.is_array()) throw json::ParseError("lighting_schedule must be an array");
    lighting::LightingSchedule schedule;
    for (const auto& item : value.as_array()) {
        lighting::LightingScheduleSegment segment;
        segment.start_of_day.value = number_field(item, "start_seconds");
        segment.end_of_day.value = number_field(item, "end_seconds");
        segment.ppfd.value = number_field(item, "ppfd_umol_per_m2_s");
        schedule.segments.push_back(segment);
    }
    return schedule;
}

json::Value root_zone_json(const rootzone::RootZoneState& value) {
    std::map<std::string, json::Value> obj;
    
    // Schema version
    obj["schema_version"] = json::Value(2.0);
    obj["id"] = string_value(value.id);
    obj["type"] = string_value(value.type == rootzone::RootZoneType::Substrate ? "Substrate" : "Reservoir");
    
    // Core parameters
    obj["substrate_bulk_volume_m3"] = number(value.substrate_bulk_volume.value);
    if (value.max_stored_water.has_value()) {
        obj["max_stored_water_m3"] = number(value.max_stored_water->value);
    }
    
    // State
    obj["interval_start_water_volume_m3"] = number(value.interval_start_water_volume.value);
    obj["current_water_volume_m3"] = number(value.current_water_volume.value);
    
    if (value.volumetric_water_content.has_value()) {
        obj["volumetric_water_content_m3_m3"] = number(value.volumetric_water_content.value());
    }
    if (value.storage_fraction.has_value()) {
        obj["storage_fraction"] = number(value.storage_fraction.value());
    }
    
    // Ledgers
    obj["cumulative_irrigation_top_off_m3"] = number(value.cumulative_irrigation_top_off.value);
    obj["cumulative_external_return_flow_m3"] = number(value.cumulative_external_return_flow.value);
    obj["cumulative_realized_withdrawal_m3"] = number(value.cumulative_realized_withdrawal.value);
    obj["cumulative_drainage_discharge_m3"] = number(value.cumulative_drainage_discharge.value);
    obj["cumulative_evaporation_m3"] = number(value.cumulative_evaporation.value);
    obj["cumulative_unmet_demand_m3"] = number(value.cumulative_unmet_demand.value);
    if (value.substrate_hydraulic_profile_id) obj["substrate_hydraulic_profile_id"] = string_value(*value.substrate_hydraulic_profile_id);
    if (value.hydraulic_stress_transfer_profile_id) obj["hydraulic_stress_transfer_profile_id"] = string_value(*value.hydraulic_stress_transfer_profile_id);
    if (value.explicit_unrestricted_water_access) obj["explicit_unrestricted_water_access"] = json::Value(*value.explicit_unrestricted_water_access);
    
    // Legacy placeholders
    obj["root_zone_temperature_c"] = number(value.root_zone_temperature.value);
    obj["ph"] = number(value.ph.value);
    obj["ec_ms_per_cm"] = number(value.electrical_conductivity.value);
    
    return json::Value(obj);
}

void read_root_zone(const json::Value& object, rootzone::RootZoneState& value) {
    if (auto* v = object.find("id")) value.id = v->as_string();
    if (auto* v = object.find("type")) {
        value.type = (v->as_string() == "Reservoir") ? rootzone::RootZoneType::Reservoir : rootzone::RootZoneType::Substrate;
    }
    
    if (auto* v = object.find("substrate_bulk_volume_m3")) value.substrate_bulk_volume.value = v->as_number();
    
    if (auto* v = object.find("max_stored_water_m3")) {
        value.max_stored_water = units::VolumeCubicMeters{v->as_number()};
    } else {
        value.max_stored_water = std::nullopt;
    }
    
    if (auto* v = object.find("interval_start_water_volume_m3")) value.interval_start_water_volume.value = v->as_number();
    else if (auto* c = object.find("current_water_volume_m3")) value.interval_start_water_volume.value = c->as_number();
    if (auto* v = object.find("current_water_volume_m3")) value.current_water_volume.value = v->as_number();
    
    if (auto* v = object.find("volumetric_water_content_m3_m3")) {
        value.volumetric_water_content = v->as_number();
    } else {
        value.volumetric_water_content = std::nullopt;
    }
    
    if (auto* v = object.find("storage_fraction")) {
        value.storage_fraction = v->as_number();
    } else {
        value.storage_fraction = std::nullopt;
    }
    
    value.cumulative_irrigation_top_off.value = object.find("cumulative_irrigation_top_off_m3") ? object.find("cumulative_irrigation_top_off_m3")->as_number() : 0.0;
    value.cumulative_external_return_flow.value = object.find("cumulative_external_return_flow_m3") ? object.find("cumulative_external_return_flow_m3")->as_number() : 0.0;
    value.cumulative_realized_withdrawal.value = object.find("cumulative_realized_withdrawal_m3") ? object.find("cumulative_realized_withdrawal_m3")->as_number() : 0.0;
    value.cumulative_drainage_discharge.value = object.find("cumulative_drainage_discharge_m3") ? object.find("cumulative_drainage_discharge_m3")->as_number() : 0.0;
    value.cumulative_evaporation.value = object.find("cumulative_evaporation_m3") ? object.find("cumulative_evaporation_m3")->as_number() : 0.0;
    value.cumulative_unmet_demand.value = object.find("cumulative_unmet_demand_m3") ? object.find("cumulative_unmet_demand_m3")->as_number() : 0.0;
    
    if (auto* v = object.find("substrate_hydraulic_profile_id"); v && v->is_string()) value.substrate_hydraulic_profile_id = v->as_string();
    if (auto* v = object.find("hydraulic_stress_transfer_profile_id"); v && v->is_string()) value.hydraulic_stress_transfer_profile_id = v->as_string();
    if (auto* v = object.find("explicit_unrestricted_water_access"); v && v->is_boolean()) value.explicit_unrestricted_water_access = v->as_boolean();
    
    value.root_zone_temperature.value = object.find("root_zone_temperature_c") ? object.find("root_zone_temperature_c")->as_number() : 20.0;
    value.ph.value = object.find("ph") ? object.find("ph")->as_number() : 6.0;
    value.electrical_conductivity.value = object.find("ec_ms_per_cm") ? object.find("ec_ms_per_cm")->as_number() : 1.5;
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
        {"root_zone_id", string_value(plant.root_zone_id)},
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
            {"gas_exchange", json::Value({
                {"net_assimilation", number(plant.latent.gas_exchange.net_assimilation.value)},
                {"intercellular_co2", number(plant.latent.gas_exchange.intercellular_co2.value)},
                {"stomatal_conductance", number(plant.latent.gas_exchange.stomatal_conductance.value)},
                {"status", number(static_cast<double>(plant.latent.gas_exchange.status))},
                {"profile_id", string_value(plant.latent.gas_exchange.profile_id)}
            })},
            {"transpiration", json::Value({
                {"flux_mol_m2_s", number(plant.latent.transpiration.flux_mol_m2_s)},
                {"total_conductance_mol_m2_s", number(plant.latent.transpiration.total_conductance_mol_m2_s)},
                {"leaf_air_vapor_gradient_mol_mol", number(plant.latent.transpiration.leaf_air_vapor_gradient_mol_mol)},
                {"status", string_value(plant.latent.transpiration.status)}
            })},
            {"effective_leaf_area_m2", optional_number(plant.latent.effective_leaf_area_m2)},
            {"leaf_characteristic_dimension_m", optional_number(plant.latent.leaf_characteristic_dimension_m)},
            {"requested_water_mol", number(plant.latent.requested_water_mol)},
            {"realized_water_mol", number(plant.latent.realized_water_mol)},
            {"unmet_demand_mol", number(plant.latent.unmet_demand_mol)}
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
    
    const json::Value* rz = object.find("root_zone_id");
    if (rz != nullptr && rz->is_string()) {
        plant.root_zone_id = rz->as_string();
    } else {
        // Fallback for legacy format that embedded root_zone objects in plants
        const json::Value* rz_obj = object.find("root_zone");
        if (rz_obj != nullptr && rz_obj->is_object()) {
            plant.root_zone_id = string_field(*rz_obj, "id");
        } else {
            plant.root_zone_id = "rootzone_" + plant.id;
        }
    }

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
    const auto& latent = object.require("latent");
    plant.latent.growth_stage = stage_from_string(string_field(latent, "growth_stage"));
    const auto* ge_ptr = latent.is_object() ? latent.find("gas_exchange") : nullptr;
    if (ge_ptr) {
        const auto& ge = *ge_ptr;
        plant.latent.gas_exchange.net_assimilation.value = number_field(ge, "net_assimilation");
        plant.latent.gas_exchange.intercellular_co2.value = number_field(ge, "intercellular_co2");
        plant.latent.gas_exchange.stomatal_conductance.value = number_field(ge, "stomatal_conductance");
        plant.latent.gas_exchange.status = static_cast<gasexchange::ConvergenceStatus>(number_field(ge, "status"));
        plant.latent.gas_exchange.profile_id = string_field(ge, "profile_id");
    }
    const auto* trans_ptr = latent.is_object() ? latent.find("transpiration") : nullptr;
    if (trans_ptr) {
        const auto& trans = *trans_ptr;
        plant.latent.transpiration.flux_mol_m2_s = number_field(trans, "flux_mol_m2_s");
        plant.latent.transpiration.total_conductance_mol_m2_s = number_field(trans, "total_conductance_mol_m2_s");
        plant.latent.transpiration.leaf_air_vapor_gradient_mol_mol = number_field(trans, "leaf_air_vapor_gradient_mol_mol");
        plant.latent.transpiration.status = string_field(trans, "status");
    }
    if (latent.is_object()) {
        const auto* el = latent.find("effective_leaf_area_m2");
        if (el && !el->is_null()) plant.latent.effective_leaf_area_m2 = el->as_number();
        const auto* lcd = latent.find("leaf_characteristic_dimension_m");
        if (lcd && !lcd->is_null()) plant.latent.leaf_characteristic_dimension_m = lcd->as_number();
        const auto* rq = latent.find("requested_water_mol");
        if (rq) plant.latent.requested_water_mol = rq->as_number();
        const auto* rl = latent.find("realized_water_mol");
        if (rl) plant.latent.realized_water_mol = rl->as_number();
        const auto* ud = latent.find("unmet_demand_mol");
        if (ud) plant.latent.unmet_demand_mol = ud->as_number();
    }
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
        {"lighting_schedule", schedule_json(cell.lighting_schedule)},
        {"lighting", lighting_json(cell.lighting)},
    });
}

void read_cell(const json::Value& object, RoomCellState& cell) {
    cell.id = string_field(object, "id");
    cell.center_x.value = number_field(object, "center_x_m");
    cell.center_y.value = number_field(object, "center_y_m");
    read_environment(object.require("environment"), cell.environment);
    cell.lighting_schedule = read_schedule(object.require("lighting_schedule"));
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
        {"uses_explicit_cells", json::Value(room.uses_explicit_cells)},
        {"cells", json::Value(std::move(cells))},
    });
}

void read_room(const json::Value& object, RoomState& room) {
    room.id = string_field(object, "id");
    room.width.value = number_field(object, "width_m");
    room.depth.value = number_field(object, "depth_m");
    room.cell_size.value = number_field(object, "cell_size_m");
    room.uses_explicit_cells = bool_field(object, "uses_explicit_cells");
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
    json::Value::Array root_zones;
    for (const cannaville::rootzone::RootZoneState& rz : state.root_zones) root_zones.push_back(root_zone_json(rz));
    json::Value::Array plant_values;
    for (const cannaville::plants::PlantState& plant : state.plants) plant_values.push_back(plant_json(plant));

    json::Value::Array water_events;
    for (const auto& ev : state.water_events) {
        water_events.push_back(json::Value({
            {"id", string_value(ev.id)},
            {"timestamp", number(ev.timestamp.value)},
            {"root_zone_id", string_value(ev.root_zone_id)},
            {"type", string_value(ev.type)},
            {"amount", number(ev.amount.value)}
        }));
    }

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
        {"root_zones", json::Value(std::move(root_zones))},
        {"plants", json::Value(std::move(plant_values))},
        {"water_events", json::Value(std::move(water_events))}
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
    const json::Value* root_zones = root.find("root_zones");
    if (root_zones != nullptr) {
        if (!root_zones->is_array()) throw json::ParseError("serialized root_zones must be an array");
        for (const auto& value : root_zones->as_array()) {
            cannaville::rootzone::RootZoneState rz;
            read_root_zone(value, rz);
            state.root_zones.push_back(std::move(rz));
        }
    }
    const auto& plants = root.require("plants");
    if (!plants.is_array()) throw json::ParseError("serialized plants must be an array");
    for (const auto& value : plants.as_array()) {
        plants::PlantState plant;
        read_plant(value, plant);
        state.plants.push_back(std::move(plant));
    }
    const json::Value* events = root.find("water_events");
    if (events != nullptr && events->is_array()) {
        for (const auto& ev_val : events->as_array()) {
            ScenarioWaterEvent ev;
            ev.id = string_field(ev_val, "id");
            ev.timestamp.value = number_field(ev_val, "timestamp");
            ev.root_zone_id = string_field(ev_val, "root_zone_id");
            ev.type = string_field(ev_val, "type");
            ev.amount.value = number_field(ev_val, "amount");
            state.water_events.push_back(std::move(ev));
        }
    }
    return state;
}

} // namespace cannaville::core
