#include <iostream>
#include "Simulation/Core/Simulation.hpp"

#include "Simulation/Core/DeterministicRng.hpp"
#include "Simulation/Core/Serialization.hpp"
#include "Simulation/GasExchange/GasExchange.hpp"
#include "Simulation/RootZone/HydraulicLimitation.hpp"
#include "Simulation/RootZone/RootZoneWater.hpp"
#include "Simulation/Transpiration/Transpiration.hpp"
#include "Simulation/LeafEnergy/LeafEnergy.hpp"
#include <unordered_map>

#include "Simulation/Lighting/LightingSystem.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace cannaville::core {

namespace {

template <typename T>
T clamp_index(T value, T upper_exclusive) {
    return std::min(value, static_cast<T>(upper_exclusive - static_cast<T>(1)));
}

std::size_t cell_index_for(const RoomState& room, units::Meters x, units::Meters y) {
    const std::size_t x_count = static_cast<std::size_t>(std::ceil(room.width.value / room.cell_size.value));
    const std::size_t y_count = static_cast<std::size_t>(std::ceil(room.depth.value / room.cell_size.value));
    const auto raw_x = static_cast<std::size_t>(std::floor(std::max(0.0, x.value) / room.cell_size.value));
    const auto raw_y = static_cast<std::size_t>(std::floor(std::max(0.0, y.value) / room.cell_size.value));
    if (room.uses_explicit_cells || room.cells.size() != x_count * y_count) {
        std::size_t best_index = 0;
        double best_distance = std::numeric_limits<double>::max();
        for (std::size_t index = 0; index < room.cells.size(); ++index) {
            const double dx = room.cells[index].center_x.value - x.value;
            const double dy = room.cells[index].center_y.value - y.value;
            const double distance = (dx * dx) + (dy * dy);
            if (distance < best_distance) {
                best_distance = distance;
                best_index = index;
            }
        }
        return best_index;
    }
    const std::size_t cell_x = clamp_index(raw_x, x_count);
    const std::size_t cell_y = clamp_index(raw_y, y_count);
    return cell_y * x_count + cell_x;
}

void sample_plant_from_room(plants::PlantState& plant, const RoomState& room) {
    if (room.cells.empty()) return;
    const RoomCellState& cell = room.cells.at(cell_index_for(room, plant.location.x, plant.location.y));
    plant.sampled_environment = cell.environment;
    plant.sampled_lighting = cell.lighting;
    if (plant.sampled_lighting.incident_shortwave_w_m2.has_value()) {
        plant.sampled_environment.incident_shortwave_w_m2 = plant.sampled_lighting.incident_shortwave_w_m2;
    }
}

std::optional<lighting::LightingScheduleSegment> scheduled_light_at(
    const lighting::LightingSchedule& schedule, double simulation_seconds) {
    const double seconds_of_day = std::fmod(std::max(0.0, simulation_seconds), lighting::kSimulationDaySeconds);
    for (const auto& segment : schedule.segments) {
        if (seconds_of_day >= segment.start_of_day.value && seconds_of_day < segment.end_of_day.value) {
            return segment;
        }
    }
    return std::nullopt;
}

struct SegmentLightingContext {
    double accumulated_dark_duration_at_start_s{0.0};
    bool segment_is_dark{false};
};

std::string lighting_context_key(const RoomState& room, std::size_t cell_index) {
    return room.id + "/cell-index-" + std::to_string(cell_index);
}

std::string transpiration_status_for(gasexchange::ConvergenceStatus status) {
    switch (status) {
        case gasexchange::ConvergenceStatus::MissingGasExchangeProfile: return "missing_gas_exchange_profile";
        case gasexchange::ConvergenceStatus::MissingSubstrateHydraulicProfile: return "missing_substrate_hydraulic_profile";
        case gasexchange::ConvergenceStatus::MissingHydraulicStressProfile: return "missing_hydraulic_stress_profile";
        case gasexchange::ConvergenceStatus::HydraulicStateUnavailable: return "hydraulic_state_unavailable";
        case gasexchange::ConvergenceStatus::MissingLeafTemperature: return "missing_leaf_temperature";
        case gasexchange::ConvergenceStatus::MissingLeafVPD: return "missing_leaf_vpd";
        case gasexchange::ConvergenceStatus::MissingAirflow: return "missing_airflow";
        case gasexchange::ConvergenceStatus::MissingLeafCharacteristicDimension: return "missing_leaf_characteristic_dimension";
        case gasexchange::ConvergenceStatus::MissingEffectiveTranspiringLeafArea: return "missing_effective_transpiring_leaf_area";
        case gasexchange::ConvergenceStatus::MissingHydraulicBeta: return "missing_hydraulic_beta";
        case gasexchange::ConvergenceStatus::FailedToConverge: return "gas_exchange_failed_to_converge";
        case gasexchange::ConvergenceStatus::NegativeVPD: return "negative_vpd";
        default: return "unavailable";
    }
}

consequences::DevelopmentStage consequence_stage_for(plants::GrowthStage stage) {
    switch (stage) {
        case plants::GrowthStage::Seedling: return consequences::DevelopmentStage::Seedling;
        case plants::GrowthStage::Vegetative: return consequences::DevelopmentStage::Vegetative;
        case plants::GrowthStage::Flowering: return consequences::DevelopmentStage::Flowering;
        case plants::GrowthStage::Harvest: return consequences::DevelopmentStage::HarvestReady;
    }
    return consequences::DevelopmentStage::Seedling;
}

plants::GrowthStage plant_stage_for(consequences::DevelopmentStage stage) {
    switch (stage) {
        case consequences::DevelopmentStage::Seedling: return plants::GrowthStage::Seedling;
        case consequences::DevelopmentStage::Vegetative: return plants::GrowthStage::Vegetative;
        case consequences::DevelopmentStage::Flowering: return plants::GrowthStage::Flowering;
        case consequences::DevelopmentStage::HarvestReady: return plants::GrowthStage::Harvest;
    }
    return plants::GrowthStage::Seedling;
}

} // namespace

Simulation::Simulation(const Scenario& scenario, std::uint64_t seed) {
    initialize(scenario, seed);
}

void Simulation::initialize(const Scenario& scenario, std::uint64_t seed) {
    const std::vector<std::string> errors = scenario.validate();
    if (!errors.empty()) {
        throw std::invalid_argument("cannot initialize invalid scenario: " + errors.front());
    }

    scenario_ = scenario;
    state_ = SimulationState{};
    last_step_segment_trace_.clear();
    state_.config.simulation_version = scenario.simulation_version;
    state_.config.fixed_timestep = scenario.fixed_timestep;
    state_.scenario_id = scenario.scenario_id;
    state_.stochastic.seed = seed;
    state_.water_events = scenario.water_events;

    for (const ScenarioRoomDefinition& definition : scenario.rooms) {
        RoomState room;
        room.id = definition.id;
        room.width = definition.width;
        room.depth = definition.depth;
        room.cell_size = definition.cell_size;
        room.uses_explicit_cells = !definition.cells.empty();
        const std::size_t x_count = static_cast<std::size_t>(std::ceil(room.width.value / room.cell_size.value));
        const std::size_t y_count = static_cast<std::size_t>(std::ceil(room.depth.value / room.cell_size.value));
        room.cells.reserve(x_count * y_count);
        if (!definition.cells.empty()) {
            for (const ScenarioCellDefinition& definition_cell : definition.cells) {
                RoomCellState cell;
                cell.id = definition_cell.id;
                cell.center_x = definition_cell.center_x;
                cell.center_y = definition_cell.center_y;
                cell.environment = definition_cell.environment;
                cell.lighting_schedule = definition_cell.lighting_schedule;
                lighting::initialize_state(cell.lighting, cell.lighting_schedule, 0.0);
                room.cells.push_back(std::move(cell));
            }
        } else {
            for (std::size_t y = 0; y < y_count; ++y) {
                for (std::size_t x = 0; x < x_count; ++x) {
                    RoomCellState cell;
                    cell.id = room.id + "/cell-" + std::to_string(x) + "-" + std::to_string(y);
                    cell.center_x.value = (static_cast<double>(x) + 0.5) * room.cell_size.value;
                    cell.center_y.value = (static_cast<double>(y) + 0.5) * room.cell_size.value;
                    lighting::initialize_state(cell.lighting, cell.lighting_schedule, 0.0);
                    room.cells.push_back(std::move(cell));
                }
            }
        }
        state_.rooms.push_back(std::move(room));
    }

    for (const ScenarioRootZoneDefinition& definition : scenario.root_zones) {
        rootzone::RootZoneState rz;
        rz.id = definition.id;
        rz.type = (definition.type == "Reservoir") ? rootzone::RootZoneType::Reservoir : rootzone::RootZoneType::Substrate;
        rz.substrate_bulk_volume = definition.substrate_bulk_volume;
        rz.max_stored_water = definition.max_stored_water;
        rz.current_water_volume = definition.initial_water_volume;
        rz.interval_start_water_volume = definition.initial_water_volume;
        rz.substrate_hydraulic_profile_id = definition.substrate_hydraulic_profile_id;
        rz.hydraulic_stress_transfer_profile_id = definition.hydraulic_stress_transfer_profile_id;
        rz.explicit_unrestricted_water_access = definition.explicit_unrestricted_water_access;
        
        if (rz.type == rootzone::RootZoneType::Substrate) {
            rootzone::SubstrateContainer container(rz.substrate_bulk_volume, rz.current_water_volume, rz.max_stored_water);
            rz.volumetric_water_content = container.volumetric_water_content();
            rz.storage_fraction = container.storage_fraction();
            
            if (rz.substrate_hydraulic_profile_id.has_value() && rz.hydraulic_stress_transfer_profile_id.has_value()) {
                auto sub_prof = rootzone::get_missing_substrate_profile();
                if (*rz.substrate_hydraulic_profile_id == "synthetic_test_a") sub_prof = rootzone::get_synthetic_substrate_test_profile_a();
                else if (*rz.substrate_hydraulic_profile_id == "synthetic_test_b") sub_prof = rootzone::get_synthetic_substrate_test_profile_b();
                
                auto stress_prof = rootzone::get_missing_stress_profile();
                if (*rz.hydraulic_stress_transfer_profile_id == "synthetic_stress_test") stress_prof = rootzone::get_synthetic_stress_test_profile();

                if (sub_prof.is_valid() && stress_prof.is_valid()) {
                    double current_vwc = rz.volumetric_water_content.value_or(0.0);
                    auto hydraulic_state = rootzone::compute_substrate_hydraulic_limitation(current_vwc, sub_prof, stress_prof);
                    rz.matric_potential = hydraulic_state.matric_potential_mpa;
                    rz.hydraulic_status = rootzone::to_string(hydraulic_state.status);
                } else {
                    rz.hydraulic_status = "MissingProfiles";
                }
            } else {
                rz.hydraulic_status = "NoProfileSpecified";
            }
        } else {
            rootzone::HydroponicReservoir res(rz.max_stored_water.value_or(units::VolumeCubicMeters{0.0}), rz.current_water_volume);
            rz.volumetric_water_content = std::nullopt;
            rz.storage_fraction = res.storage_fraction();
            rz.matric_potential = std::nullopt;
            
            bool has_sufficient_water = rz.current_water_volume.value > 0.0;
            bool explicit_access = rz.explicit_unrestricted_water_access.value_or(false);
            auto hydraulic_state = rootzone::compute_dwc_hydraulic_limitation(has_sufficient_water, false, explicit_access);
            rz.hydraulic_status = rootzone::to_string(hydraulic_state.status);
        }
        
        state_.root_zones.push_back(std::move(rz));
    }

    for (const ScenarioPlantDefinition& definition : scenario.plants) {
        plants::PlantState plant;
        plant.id = definition.id;
        plant.location.room_id = definition.room_id;
        plant.root_zone_id = definition.root_zone_id;
        plant.location.x = definition.x;
        plant.location.y = definition.y;
        plant.location.z = definition.z;
        plant.genetics.cultivar_id = definition.cultivar_id;
        plant.stochastic.stream_seed = seed;
        plant.stochastic.stream_state = seed;
        if (definition.gas_exchange_profile_id) plant.latent.gas_exchange.profile_id = *definition.gas_exchange_profile_id;
        if (definition.effective_transpiring_leaf_area_m2) plant.latent.effective_leaf_area_m2 = definition.effective_transpiring_leaf_area_m2;
        if (definition.leaf_characteristic_dimension_m) plant.latent.leaf_characteristic_dimension_m = definition.leaf_characteristic_dimension_m;
        plant.leaf_energy_profile = definition.leaf_energy_profile;
        plant.consequence_profile = definition.consequence_profile;
        plant.consequence_state.model_version = plant.consequence_profile.model_version;
        plant.consequence_state.stage = consequence_stage_for(plant.latent.growth_stage);
        const auto room_it = std::find_if(state_.rooms.begin(), state_.rooms.end(), [&](const RoomState& room) {
            return room.id == plant.location.room_id;
        });
        if (room_it == state_.rooms.end()) {
            throw std::logic_error("validated plant room was not initialized");
        }
        sample_plant_from_room(plant, *room_it);
        state_.plants.push_back(std::move(plant));
    }
    rng_ = DeterministicRng(seed);
    state_.stochastic.rng_state = rng_.state();
}

void Simulation::submit_player_action(const PlayerAction& /*action*/) {
    // Action routing is intentionally an inert boundary in the bootstrap.
    // Future actions must be validated and applied by explicit model systems.
}

void Simulation::advance_fixed_step() {
    last_step_segment_trace_.clear();
    if (state_.rooms.empty()) {
        state_.clock.elapsed.value += state_.config.fixed_timestep.value;
        ++state_.clock.steps.value;
        return;
    }

    const double step_start = state_.clock.elapsed.value;
    const double timestep = state_.config.fixed_timestep.value;
    const double step_end = step_start + timestep;

    std::vector<double> boundaries;
    boundaries.push_back(step_start);
    boundaries.push_back(step_end);

    for (const RoomState& room : state_.rooms) {
        for (const RoomCellState& cell : room.cells) {
            double start_of_day = std::fmod(step_start, 86400.0);
            
            for (const auto& segment : cell.lighting_schedule.segments) {
                double b1 = step_start + (segment.start_of_day.value - start_of_day);
                if (segment.start_of_day.value < start_of_day) b1 += 86400.0;
                if (b1 > step_start && b1 < step_end) boundaries.push_back(b1);
                
                double b2 = step_start + (segment.end_of_day.value - start_of_day);
                if (segment.end_of_day.value < start_of_day) b2 += 86400.0;
                if (b2 > step_start && b2 < step_end) boundaries.push_back(b2);
            }
        }
    }

    for (const auto& ev : scenario_.water_events) {
        if (ev.timestamp.value >= step_start && ev.timestamp.value < step_end) {
            boundaries.push_back(ev.timestamp.value);
        }
    }

    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());

    double current_time = step_start;

    for (size_t b_idx = 1; b_idx < boundaries.size(); ++b_idx) {
        double next_boundary = boundaries[b_idx];
        double segment_duration = next_boundary - current_time;
        if (segment_duration <= 0.0) continue;

        std::unordered_map<std::string, double> segment_water_inputs;
        for (const auto& ev : scenario_.water_events) {
            if (std::abs(ev.timestamp.value - current_time) < 1e-9) {
                for (auto& rz : state_.root_zones) {
                    if (rz.id == ev.root_zone_id) {
                        if (rz.type == rootzone::RootZoneType::Substrate) {
                            rootzone::SubstrateContainer container(rz.substrate_bulk_volume, rz.current_water_volume, rz.max_stored_water);
                            if (ev.type == "irrigation") {
                                container.irrigate(ev.amount);
                            } else if (ev.type == "top_off") {
                                container.add_top_off(ev.amount);
                            } else if (ev.type == "external_return") {
                                container.add_external_return_flow(ev.amount);
                            } else if (ev.type == "drainage") {
                                container.process_drainage(ev.amount);
                            }
                            container.process_drainage(std::nullopt); // apply capacity
                            auto balance = container.get_last_balance();
                            segment_water_inputs[rz.id] += balance.irrigation_added_m3 + balance.top_off_added_m3 + balance.external_return_flow_added_m3;
                            rz.cumulative_irrigation_top_off.value += balance.irrigation_added_m3 + balance.top_off_added_m3;
                            rz.cumulative_external_return_flow.value += balance.external_return_flow_added_m3;
                            rz.cumulative_drainage_discharge.value += balance.drainage_removed_m3 + balance.discharge_removed_m3;
                            rz.current_water_volume = container.current_water();
                            rz.volumetric_water_content = container.volumetric_water_content();
                            rz.storage_fraction = container.storage_fraction();
                        } else {
                            rootzone::HydroponicReservoir res(rz.max_stored_water.value_or(units::VolumeCubicMeters{0.0}), rz.current_water_volume);
                            if (ev.type == "top_off" || ev.type == "irrigation") {
                                res.add_top_off(ev.amount);
                            } else if (ev.type == "external_return") {
                                res.add_external_return_flow(ev.amount);
                            } else if (ev.type == "discharge" || ev.type == "drainage") {
                                res.process_discharge(ev.amount);
                            }
                            res.process_discharge(std::nullopt); // apply capacity
                            auto balance = res.get_last_balance();
                            segment_water_inputs[rz.id] += balance.top_off_added_m3 + balance.external_return_flow_added_m3;
                            rz.cumulative_irrigation_top_off.value += balance.top_off_added_m3;
                            rz.cumulative_external_return_flow.value += balance.external_return_flow_added_m3;
                            rz.cumulative_drainage_discharge.value += balance.discharge_removed_m3 + balance.drainage_removed_m3;
                            rz.current_water_volume = res.current_water();
                            rz.storage_fraction = res.storage_fraction();
                        }
                    }
                }
            }
        }

        std::unordered_map<std::string, SegmentLightingContext> segment_lighting_context;
        for (const RoomState& room : state_.rooms) {
            for (std::size_t cell_index = 0; cell_index < room.cells.size(); ++cell_index) {
                const RoomCellState& cell = room.cells[cell_index];
                const auto scheduled_light = scheduled_light_at(cell.lighting_schedule, current_time);
                segment_lighting_context[lighting_context_key(room, cell_index)] = SegmentLightingContext{
                    cell.lighting.accumulated_dark_duration.value,
                    scheduled_light.has_value() && scheduled_light->ppfd.value <= 0.0};
            }
        }

        // Lighting
        for (RoomState& room : state_.rooms) {
            for (RoomCellState& cell : room.cells) {
                lighting::advance_state(cell.lighting, cell.lighting_schedule, current_time, segment_duration);
            }
        }

        std::unordered_map<std::string, std::optional<double>> root_zone_betas;
        for (rootzone::RootZoneState& rz : state_.root_zones) {
            if (rz.type == rootzone::RootZoneType::Substrate) {
                if (rz.substrate_hydraulic_profile_id.has_value() && rz.hydraulic_stress_transfer_profile_id.has_value()) {
                    auto sub_prof = rootzone::get_missing_substrate_profile();
                    if (*rz.substrate_hydraulic_profile_id == "synthetic_test_a") sub_prof = rootzone::get_synthetic_substrate_test_profile_a();
                    else if (*rz.substrate_hydraulic_profile_id == "synthetic_test_b") sub_prof = rootzone::get_synthetic_substrate_test_profile_b();
                    
                    auto stress_prof = rootzone::get_missing_stress_profile();
                    if (*rz.hydraulic_stress_transfer_profile_id == "synthetic_stress_test") stress_prof = rootzone::get_synthetic_stress_test_profile();

                    if (sub_prof.is_valid() && stress_prof.is_valid()) {
                        double current_vwc = rz.substrate_bulk_volume.value > 0.0 ? rz.current_water_volume.value / rz.substrate_bulk_volume.value : 0.0;
                        auto hydraulic_state = rootzone::compute_substrate_hydraulic_limitation(current_vwc, sub_prof, stress_prof);
                        root_zone_betas[rz.id] = hydraulic_state.beta_hydraulic;
                        rz.matric_potential = hydraulic_state.matric_potential_mpa;
                        rz.hydraulic_status = rootzone::to_string(hydraulic_state.status);
                    }
                }
            } else {
                bool has_sufficient_water = rz.current_water_volume.value > 0.0;
                bool explicit_access = rz.explicit_unrestricted_water_access.value_or(false);
                auto hydraulic_state = rootzone::compute_dwc_hydraulic_limitation(has_sufficient_water, false, explicit_access);
                if (explicit_access) {
                    root_zone_betas[rz.id] = hydraulic_state.beta_hydraulic;
                }
                rz.hydraulic_status = rootzone::to_string(hydraulic_state.status);
            }
        }

        std::unordered_map<std::string, std::vector<rootzone::TranspirationRequest>> requests_by_rz;
        std::unordered_map<std::string, std::vector<plants::PlantState*>> plants_by_rz;
        std::unordered_map<std::string, std::size_t> diagnostic_index_by_plant;

        for (plants::PlantState& plant : state_.plants) {
            const auto room_it = std::find_if(state_.rooms.begin(), state_.rooms.end(), [&](const RoomState& room) {
                return room.id == plant.location.room_id;
            });
            if (room_it != state_.rooms.end()) {
                sample_plant_from_room(plant, *room_it);
                if (!room_it->cells.empty()) {
                    const std::size_t cell_index = cell_index_for(*room_it, plant.location.x, plant.location.y);
                    const RoomCellState& sampled_cell = room_it->cells.at(cell_index);
                    const auto scheduled_light = scheduled_light_at(sampled_cell.lighting_schedule, current_time);
                    if (scheduled_light.has_value()) {
                        plant.sampled_lighting.ppfd = scheduled_light->ppfd;
                        plant.sampled_lighting.light_on = scheduled_light->ppfd.value > 0.0;
                        plant.sampled_lighting.incident_shortwave_w_m2 = scheduled_light->incident_shortwave_w_m2;
                        plant.sampled_environment.incident_shortwave_w_m2 = scheduled_light->incident_shortwave_w_m2.has_value()
                            ? scheduled_light->incident_shortwave_w_m2
                            : sampled_cell.environment.incident_shortwave_w_m2;
                    }
                }
            }

            gasexchange::ConvergenceStatus fail_status = gasexchange::ConvergenceStatus::NotRun;
            const auto ge_profile = gasexchange::get_profile(plant.latent.gas_exchange.profile_id);
            plant.latent.boundary_layer = transpiration::BoundaryLayerConductance{};
            plant.latent.transpiration = transpiration::TranspirationResult{};
            plant.latent.leaf_energy = leafenergy::LeafEnergyState{};
            plant.latent.leaf_energy.model_version = plant.leaf_energy_profile.model_version;
            plant.latent.requested_water_mol = 0.0;
            plant.latent.realized_water_mol = 0.0;
            plant.latent.unmet_demand_mol = 0.0;
            plant.latent.gas_exchange.profile_id = ge_profile.id;
            plant.latent.gas_exchange.g1_reference = ge_profile.is_configured ? ge_profile.medlyn.g1 : 0.0;
            plant.latent.gas_exchange.g1_effective = 0.0;

            std::optional<double> beta = std::nullopt;
            if (root_zone_betas.count(plant.root_zone_id)) beta = root_zone_betas[plant.root_zone_id];

            if (!ge_profile.is_configured) {
                fail_status = gasexchange::ConvergenceStatus::MissingGasExchangeProfile;
            } else if (!beta.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingHydraulicBeta;
                for (const auto& rz : state_.root_zones) {
                    if (rz.id != plant.root_zone_id) continue;
                    if (rz.type == rootzone::RootZoneType::Substrate) {
                        if (!rz.substrate_hydraulic_profile_id.has_value()) fail_status = gasexchange::ConvergenceStatus::MissingSubstrateHydraulicProfile;
                        else if (!rz.hydraulic_stress_transfer_profile_id.has_value()) fail_status = gasexchange::ConvergenceStatus::MissingHydraulicStressProfile;
                        else fail_status = gasexchange::ConvergenceStatus::HydraulicStateUnavailable;
                    } else if (!rz.explicit_unrestricted_water_access.value_or(false)) {
                        fail_status = gasexchange::ConvergenceStatus::HydraulicStateUnavailable;
                    }
                }
            } else if (!plant.sampled_environment.physics_available) {
                fail_status = gasexchange::ConvergenceStatus::MissingLeafTemperature;
            } else if (!plant.latent.effective_leaf_area_m2.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingEffectiveTranspiringLeafArea;
            } else if (!plant.latent.leaf_characteristic_dimension_m.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingLeafCharacteristicDimension;
            } else if (plant.leaf_energy_profile.temperature_mode == leafenergy::LeafTemperatureMode::ExternalLeafTemperature &&
                       !plant.sampled_environment.leaf_temperature.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingLeafTemperature;
            } else if (plant.leaf_energy_profile.temperature_mode == leafenergy::LeafTemperatureMode::ExternalLeafTemperature &&
                       !plant.sampled_environment.leaf_vpd.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingLeafVPD;
            } else if (!plant.sampled_environment.airflow.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingAirflow;
            }

            std::optional<leafenergy::LeafEnergyEvaluation> final_energy_evaluation;
            if (fail_status == gasexchange::ConvergenceStatus::NotRun) {
                leafenergy::solve_leaf_energy_balance(
                    plant.latent.leaf_energy,
                    plant.leaf_energy_profile,
                    plant.sampled_environment,
                    plant.sampled_lighting,
                    ge_profile,
                    beta,
                    plant.latent.leaf_characteristic_dimension_m);

                if (plant.latent.leaf_energy.status == "external_temperature_used") {
                    if (!plant.sampled_environment.leaf_temperature.has_value() || !plant.sampled_environment.leaf_vpd.has_value()) {
                        fail_status = gasexchange::ConvergenceStatus::MissingLeafTemperature;
                    }
                } else if (plant.latent.leaf_energy.status == "converged" && plant.latent.leaf_energy.leaf_temperature.has_value()) {
                    plant.sampled_environment.leaf_temperature = plant.latent.leaf_energy.leaf_temperature;
                    plant.sampled_environment.leaf_vpd = plant.latent.leaf_energy.leaf_vpd;
                    final_energy_evaluation = leafenergy::evaluate_leaf_energy_residual(
                        *plant.latent.leaf_energy.leaf_temperature,
                        plant.sampled_environment,
                        plant.sampled_lighting,
                        plant.leaf_energy_profile,
                        ge_profile,
                        beta,
                        plant.latent.leaf_characteristic_dimension_m);
                    if (!final_energy_evaluation->available) {
                        plant.latent.leaf_energy.status = final_energy_evaluation->status;
                        fail_status = gasexchange::ConvergenceStatus::UnsupportedBoundaryLayerRegime;
                    }
                } else {
                    if (plant.latent.leaf_energy.status == "unsupported_boundary_layer") {
                        fail_status = gasexchange::ConvergenceStatus::UnsupportedBoundaryLayerRegime;
                    } else if (plant.latent.leaf_energy.status == "missing_leaf_vpd") {
                        fail_status = gasexchange::ConvergenceStatus::MissingLeafVPD;
                    } else if (plant.latent.leaf_energy.status == "missing_airflow") {
                        fail_status = gasexchange::ConvergenceStatus::MissingAirflow;
                    } else if (plant.latent.leaf_energy.status == "missing_leaf_characteristic_dimension") {
                        fail_status = gasexchange::ConvergenceStatus::MissingLeafCharacteristicDimension;
                    } else {
                        fail_status = gasexchange::ConvergenceStatus::MissingLeafTemperature;
                    }
                }
            }

            if (fail_status == gasexchange::ConvergenceStatus::NotRun) {
                if (final_energy_evaluation.has_value()) {
                    plant.latent.gas_exchange = final_energy_evaluation->gas_exchange;
                    plant.latent.boundary_layer = final_energy_evaluation->boundary_layer;
                    plant.latent.transpiration = final_energy_evaluation->transpiration;
                } else {
                    const units::VPDKPa vpd = *plant.sampled_environment.leaf_vpd;
                    gasexchange::solve_coupled_gas_exchange(
                        plant.latent.gas_exchange, ge_profile, plant.sampled_lighting.ppfd,
                        plant.sampled_environment.carbon_dioxide, vpd,
                        plant.sampled_environment.leaf_temperature,
                        plant.sampled_environment.atmospheric_pressure, beta);
                    if (plant.latent.gas_exchange.status == gasexchange::ConvergenceStatus::Converged) {
                        const auto air_props = transpiration::calculate_air_properties(
                            plant.sampled_environment.air_temperature, plant.sampled_environment.atmospheric_pressure);
                        plant.latent.boundary_layer = transpiration::calculate_boundary_layer_conductance(
                            *plant.sampled_environment.airflow,
                            units::Meters{*plant.latent.leaf_characteristic_dimension_m}, air_props,
                            *plant.sampled_environment.leaf_temperature, plant.sampled_environment.air_temperature);
                        plant.latent.transpiration = transpiration::calculate_transpiration(
                            plant.latent.gas_exchange.stomatal_conductance, plant.latent.boundary_layer,
                            vpd, plant.sampled_environment.atmospheric_pressure);
                    }
                }
                if (plant.latent.gas_exchange.status != gasexchange::ConvergenceStatus::Converged) {
                    fail_status = plant.latent.gas_exchange.status;
                }
            }

            if (fail_status == gasexchange::ConvergenceStatus::NotRun && plant.latent.transpiration.available) {
                const double nonnegative_flux = std::max(0.0, plant.latent.transpiration.flux_mol_m2_s);
                plant.latent.requested_water_mol = nonnegative_flux * (*plant.latent.effective_leaf_area_m2) * segment_duration;
                if (nonnegative_flux > 0.0) {
                    rootzone::TranspirationRequest req;
                    req.flux = units::WaterFluxMolesPerSquareMeterSecond{nonnegative_flux};
                    req.area = units::AreaSquareMeters{*plant.latent.effective_leaf_area_m2};
                    requests_by_rz[plant.root_zone_id].push_back(req);
                    plants_by_rz[plant.root_zone_id].push_back(&plant);
                }
            } else if (fail_status == gasexchange::ConvergenceStatus::NotRun &&
                       plant.latent.gas_exchange.status == gasexchange::ConvergenceStatus::Converged) {
                // Gas exchange can be a valid trial result even when the shared
                // boundary-layer/transpiration calculation is unavailable. Keep
                // that distinction visible; no water request is authorized.
                plant.latent.transpiration.available = false;
                plant.latent.transpiration.flux_mol_m2_s = 0.0;
                plant.latent.requested_water_mol = 0.0;
                if (plant.latent.transpiration.status.empty()) {
                    plant.latent.transpiration.status = "transpiration_unavailable";
                }
            } else {
                plant.latent.gas_exchange.status = fail_status;
                plant.latent.gas_exchange.net_assimilation.value = 0.0;
                plant.latent.gas_exchange.stomatal_conductance.value = 0.0;
                plant.latent.gas_exchange.intercellular_co2.value = 0.0;
                plant.latent.transpiration.available = false;
                plant.latent.transpiration.flux_mol_m2_s = 0.0;
                plant.latent.requested_water_mol = 0.0;
                plant.latent.transpiration.status = transpiration_status_for(fail_status);
            }

            SegmentDiagnostic diagnostic;
            diagnostic.plant_id = plant.id;
            diagnostic.root_zone_id = plant.root_zone_id;
            diagnostic.start_time_s = current_time;
            diagnostic.duration_s = segment_duration;
            diagnostic.ppfd_umol_m2_s = plant.sampled_lighting.ppfd.value;
            diagnostic.water_input_before_segment_m3 = segment_water_inputs[plant.root_zone_id];
            for (const auto& rz : state_.root_zones) {
                if (rz.id == plant.root_zone_id) {
                    diagnostic.root_zone_storage_at_segment_start_m3 = rz.current_water_volume.value;
                    diagnostic.vwc_at_segment_start = rz.volumetric_water_content;
                    diagnostic.matric_potential_mpa = rz.matric_potential;
                    break;
                }
            }
            diagnostic.beta_hydraulic = plant.latent.gas_exchange.beta_hydraulic;
            diagnostic.g1_effective = plant.latent.gas_exchange.g1_effective;
            diagnostic.stomatal_conductance_mol_m2_s = plant.latent.gas_exchange.stomatal_conductance.value;
            diagnostic.net_assimilation_umol_m2_s = plant.latent.gas_exchange.net_assimilation.value;
            diagnostic.transpiration_available = plant.latent.transpiration.available;
            if (diagnostic.transpiration_available) {
                diagnostic.transpiration_flux_mol_m2_s = plant.latent.transpiration.flux_mol_m2_s;
            }
            diagnostic.requested_water_m3 = rootzone::moles_water_to_cubic_meters(plant.latent.requested_water_mol);
            diagnostic.gas_exchange_status = gasexchange::to_string(plant.latent.gas_exchange.status);
            diagnostic.transpiration_status = plant.latent.transpiration.status;
            if (plant.latent.leaf_energy.leaf_temperature) diagnostic.leaf_temperature_c = plant.latent.leaf_energy.leaf_temperature->value;
            if (plant.latent.leaf_energy.leaf_vpd) diagnostic.leaf_vpd_kpa = plant.latent.leaf_energy.leaf_vpd->value;
            diagnostic.absorbed_shortwave_w_m2 = plant.latent.leaf_energy.absorbed_shortwave_w_m2;
            diagnostic.net_longwave_w_m2 = plant.latent.leaf_energy.net_longwave_w_m2;
            diagnostic.sensible_heat_flux_w_m2 = plant.latent.leaf_energy.sensible_heat_flux_w_m2;
            diagnostic.latent_heat_flux_w_m2 = plant.latent.leaf_energy.latent_heat_flux_w_m2;
            diagnostic.energy_residual_w_m2 = plant.latent.leaf_energy.energy_residual_w_m2;
            diagnostic.leaf_energy_iterations = plant.latent.leaf_energy.solver_iterations;
            diagnostic.leaf_energy_status = plant.latent.leaf_energy.status;
            diagnostic_index_by_plant[plant.id] = last_step_segment_trace_.size();
            last_step_segment_trace_.push_back(std::move(diagnostic));
        }

        for (rootzone::RootZoneState& rz : state_.root_zones) {
            if (requests_by_rz.count(rz.id) == 0) continue;

            auto& requests = requests_by_rz[rz.id];
            auto& plants = plants_by_rz[rz.id];

            std::vector<units::VolumeCubicMeters> realized_m3_list;

            if (rz.type == rootzone::RootZoneType::Substrate) {
                rootzone::SubstrateContainer container(rz.substrate_bulk_volume, rz.current_water_volume, rz.max_stored_water);
                realized_m3_list = container.withdraw_transpiration_shared(requests, units::Seconds{segment_duration});
                rz.current_water_volume = container.current_water();
                auto balance = container.get_last_balance();
                rz.cumulative_realized_withdrawal.value += balance.plant_withdrawal_realized_m3;
                rz.cumulative_unmet_demand.value += balance.plant_withdrawal_unmet_m3;
                rz.volumetric_water_content = container.volumetric_water_content();
                rz.storage_fraction = container.storage_fraction();
            } else {
                rootzone::HydroponicReservoir res(rz.max_stored_water.value_or(units::VolumeCubicMeters{0.0}), rz.current_water_volume);
                realized_m3_list = res.withdraw_transpiration_shared(requests, units::Seconds{segment_duration});
                rz.current_water_volume = res.current_water();
                auto balance = res.get_last_balance();
                rz.cumulative_realized_withdrawal.value += balance.plant_withdrawal_realized_m3;
                rz.cumulative_unmet_demand.value += balance.plant_withdrawal_unmet_m3;
                rz.storage_fraction = res.storage_fraction();
            }

            for (size_t i = 0; i < plants.size(); ++i) {
                plants[i]->latent.realized_water_mol = rootzone::cubic_meters_water_to_moles(realized_m3_list[i].value);
                plants[i]->latent.unmet_demand_mol = std::max(0.0, plants[i]->latent.requested_water_mol - plants[i]->latent.realized_water_mol);
                auto diagnostic_it = diagnostic_index_by_plant.find(plants[i]->id);
                if (diagnostic_it != diagnostic_index_by_plant.end()) {
                    SegmentDiagnostic& diagnostic = last_step_segment_trace_[diagnostic_it->second];
                    diagnostic.realized_water_m3 = realized_m3_list[i].value;
                    diagnostic.post_segment_storage_m3 = rz.current_water_volume.value;
                }
            }
        }

        for (SegmentDiagnostic& diagnostic : last_step_segment_trace_) {
            if (diagnostic.start_time_s == current_time && diagnostic.post_segment_storage_m3 == 0.0) {
                for (const auto& rz : state_.root_zones) {
                    if (rz.id == diagnostic.root_zone_id) {
                        diagnostic.post_segment_storage_m3 = rz.current_water_volume.value;
                        break;
                    }
                }
            }
        }

        // P2 consumes the final segment result only after shared water allocation
        // has populated realized and unmet demand for every requesting plant.
        for (plants::PlantState& plant : state_.plants) {
            if (!plant.consequence_profile.is_configured) continue;
            consequences::SegmentInputs consequence_inputs;
            consequence_inputs.start_time_s = current_time;
            consequence_inputs.duration_s = segment_duration;
            if (plant.latent.gas_exchange.status == gasexchange::ConvergenceStatus::Converged) {
                consequence_inputs.net_assimilation_umol_m2_s = plant.latent.gas_exchange.net_assimilation.value;
            }
            consequence_inputs.hydraulic_beta = plant.latent.gas_exchange.beta_hydraulic;
            consequence_inputs.water_allocation_available = plant.latent.transpiration.available;
            consequence_inputs.requested_water_mol = plant.latent.requested_water_mol;
            consequence_inputs.realized_water_mol = plant.latent.realized_water_mol;
            if (plant.latent.leaf_energy.leaf_temperature.has_value()) {
                consequence_inputs.leaf_temperature_c = plant.latent.leaf_energy.leaf_temperature->value;
            } else if (plant.sampled_environment.leaf_temperature.has_value()) {
                consequence_inputs.leaf_temperature_c = plant.sampled_environment.leaf_temperature->value;
            }
            consequence_inputs.absorbed_shortwave_w_m2 = plant.latent.leaf_energy.absorbed_shortwave_w_m2;
            const auto room_it = std::find_if(state_.rooms.begin(), state_.rooms.end(), [&](const RoomState& room) {
                return room.id == plant.location.room_id;
            });
            if (room_it != state_.rooms.end() && !room_it->cells.empty()) {
                const std::size_t cell_index = cell_index_for(*room_it, plant.location.x, plant.location.y);
                const auto context_it = segment_lighting_context.find(lighting_context_key(*room_it, cell_index));
                if (context_it != segment_lighting_context.end()) {
                    consequence_inputs.accumulated_dark_duration_at_segment_start_s =
                        context_it->second.accumulated_dark_duration_at_start_s;
                    consequence_inputs.segment_is_dark = context_it->second.segment_is_dark;
                }
            }
            consequence_inputs.gas_exchange_status = gasexchange::to_string(plant.latent.gas_exchange.status);
            consequence_inputs.transpiration_status = plant.latent.transpiration.status;
            consequences::integrate_segment(
                plant.consequence_state,
                plant.consequence_presentation,
                plant.consequence_profile,
                consequence_inputs);
            plant.latent.growth_stage = plant_stage_for(plant.consequence_state.stage);
            plant.observable.displayed_growth_stage = plant.latent.growth_stage;
            plant.derived.readiness_label = plant.consequence_presentation.derived_condition;

            const auto diagnostic_it = diagnostic_index_by_plant.find(plant.id);
            if (diagnostic_it != diagnostic_index_by_plant.end()) {
                SegmentDiagnostic& diagnostic = last_step_segment_trace_[diagnostic_it->second];
                diagnostic.consequence_status = plant.consequence_state.integration_status;
                diagnostic.opportunity_signal_0_1 = plant.consequence_state.current_opportunity_signal_0_1;
                diagnostic.water_severity_0_1 = plant.consequence_state.water_current_severity_0_1;
                diagnostic.thermal_severity_0_1 = plant.consequence_state.thermal_current_severity_0_1;
                diagnostic.radiation_severity_0_1 = plant.consequence_state.radiation_current_severity_0_1;
                diagnostic.consequence_stage = consequences::to_string(plant.consequence_state.stage);
                diagnostic.consequence_stage_progress_0_1 = plant.consequence_state.stage_progress_0_1;
            }
        }

        current_time = next_boundary;
    }

    for (plants::PlantState& plant : state_.plants) {
        plant.history.elapsed.value += timestep;
        ++plant.history.steps.value;
    }
    
    state_.clock.elapsed.value += timestep;
    ++state_.clock.steps.value;
    state_.stochastic.rng_state = rng_.state();
}
OfflineReconciliation Simulation::reconcile_offline_elapsed(units::Seconds elapsed) {
    if (!std::isfinite(elapsed.value) || elapsed.value < 0.0) {
        throw std::invalid_argument("offline elapsed seconds must be finite and non-negative");
    }
    const double timestep = state_.config.fixed_timestep.value;
    const double raw_steps = std::floor((elapsed.value / timestep) + 1e-12);
    const auto step_count = static_cast<std::uint64_t>(raw_steps);
    for (std::uint64_t step = 0; step < step_count; ++step) advance_fixed_step();
    const double advanced_seconds = static_cast<double>(step_count) * timestep;
    return OfflineReconciliation{
        elapsed,
        units::Seconds{advanced_seconds},
        units::Seconds{elapsed.value - advanced_seconds},
    };
}

ObservableSimulationState Simulation::observable_state() const {
    ObservableSimulationState observable;
    observable.simulation_version = state_.config.simulation_version;
    observable.scenario_id = state_.scenario_id;
    observable.clock = state_.clock;
    observable.rooms = state_.rooms;
    observable.plants.reserve(state_.plants.size());
    for (const plants::PlantState& plant : state_.plants) {
        observable.plants.push_back(ObservablePlantState{
            plant.id,
            plant.location.room_id,
            plant.location.x,
            plant.location.y,
            plant.location.z,
            plant.observable.displayed_growth_stage,
            plant.observable.interaction_available,
            plant.derived.readiness_label,
            plant.consequence_presentation,
        });
    }
    return observable;
}

std::string Simulation::serialize_state() const {
    return serialize_state_json(state_);
}

void Simulation::load_serialized_state(std::string_view serialized) {
    state_ = deserialize_state_json(std::string(serialized));
    last_step_segment_trace_.clear();
    scenario_ = Scenario{};
    scenario_.scenario_id = state_.scenario_id;
    scenario_.water_events = state_.water_events;
    scenario_.simulation_version = state_.config.simulation_version;
    scenario_.fixed_timestep = state_.config.fixed_timestep;
    for (const RoomState& room : state_.rooms) {
        ScenarioRoomDefinition scenario_room{room.id, room.width, room.depth, room.cell_size, {}};
        for (const RoomCellState& cell : room.cells) {
            scenario_room.cells.push_back(ScenarioCellDefinition{
                cell.id,
                cell.center_x,
                cell.center_y,
                cell.environment,
                cell.lighting_schedule,
            });
        }
        scenario_.rooms.push_back(std::move(scenario_room));
    }
    for (const rootzone::RootZoneState& rz : state_.root_zones) {
        ScenarioRootZoneDefinition scenario_rz;
        scenario_rz.id = rz.id;
        scenario_rz.type = (rz.type == rootzone::RootZoneType::Reservoir) ? "Reservoir" : "Substrate";
        scenario_rz.substrate_bulk_volume = rz.substrate_bulk_volume;
        scenario_rz.max_stored_water = rz.max_stored_water;
        scenario_rz.initial_water_volume = rz.interval_start_water_volume;
        scenario_rz.substrate_hydraulic_profile_id = rz.substrate_hydraulic_profile_id;
        scenario_rz.hydraulic_stress_transfer_profile_id = rz.hydraulic_stress_transfer_profile_id;
        scenario_rz.explicit_unrestricted_water_access = rz.explicit_unrestricted_water_access;
        scenario_.root_zones.push_back(std::move(scenario_rz));
    }
    for (const plants::PlantState& plant : state_.plants) {
        scenario_.plants.push_back(ScenarioPlantDefinition{
            plant.id,
            plant.location.room_id,
            plant.genetics.cultivar_id,
            plant.root_zone_id,
            plant.location.x,
            plant.location.y,
            plant.location.z,
            plant.latent.gas_exchange.profile_id,
            plant.latent.effective_leaf_area_m2,
            plant.latent.leaf_characteristic_dimension_m,
            plant.leaf_energy_profile,
            plant.consequence_profile
        });
    }
    rng_.restore_state(state_.stochastic.seed, state_.stochastic.rng_state);
}

const Scenario& Simulation::scenario() const {
    return scenario_;
}

const SimulationState& Simulation::full_state_for_internal_use() const {
    return state_;
}

std::uint64_t Simulation::seed() const {
    return state_.stochastic.seed;
}

std::string Simulation::csv_header() const {
    return "simulation_version,scenario_id,seed,step_index,simulation_timestamp_seconds,room_id,cell_id,"
           "air_temperature_c,relative_humidity_percent,saturation_vapor_pressure_kpa,"
           "actual_vapor_pressure_kpa,air_vpd_kpa,leaf_temperature_c,leaf_vpd_kpa,"
           "ppfd_umol_per_m2_s,dli_mol_per_m2_day,accumulated_light_on_duration_s,"
           "accumulated_dark_duration_s,co2_umol_per_mol,incident_shortwave_w_m2,radiative_surroundings_temperature_c\n";
}

std::string Simulation::csv_row() const {
    std::string output;
    for (const RoomState& room : state_.rooms) {
        for (const RoomCellState& cell : room.cells) {
            const auto optional_field = [](const std::optional<double>& value) {
                return value.has_value() ? std::to_string(*value) : std::string{};
            };
            const std::optional<double> leaf_temperature = cell.environment.leaf_temperature.has_value()
                ? std::optional<double>(cell.environment.leaf_temperature->value)
                : std::nullopt;
            const std::optional<double> leaf_vpd = cell.environment.leaf_vpd.has_value()
                ? std::optional<double>(cell.environment.leaf_vpd->value)
                : std::nullopt;
            const std::optional<double> shortwave = cell.lighting.incident_shortwave_w_m2.has_value()
                ? cell.lighting.incident_shortwave_w_m2
                : cell.environment.incident_shortwave_w_m2;
            const std::optional<double> radiative = cell.environment.radiative_surroundings_temperature.has_value()
                ? std::optional<double>(cell.environment.radiative_surroundings_temperature->value)
                : std::nullopt;
            const std::string derived = cell.environment.physics_available
                ? std::to_string(cell.environment.saturation_vapor_pressure.value) + "," +
                    std::to_string(cell.environment.actual_vapor_pressure.value) + "," +
                    std::to_string(cell.environment.air_vpd.value)
                : ",,";
            output += state_.config.simulation_version + "," + state_.scenario_id + "," +
                std::to_string(state_.stochastic.seed) + "," +
                std::to_string(state_.clock.steps.value) + "," +
                std::to_string(state_.clock.elapsed.value) + "," +
                room.id + "," + cell.id + "," +
                std::to_string(cell.environment.air_temperature.value) + "," +
                std::to_string(cell.environment.relative_humidity.value) + "," +
                derived + "," + optional_field(leaf_temperature) + "," + optional_field(leaf_vpd) + "," +
                std::to_string(cell.lighting.ppfd.value) + "," +
                std::to_string(cell.lighting.dli.value) + "," +
                std::to_string(cell.lighting.accumulated_light_on_duration.value) + "," +
                std::to_string(cell.lighting.accumulated_dark_duration.value) + "," +
                std::to_string(cell.environment.carbon_dioxide.value) + "," +
                optional_field(shortwave) + "," + optional_field(radiative) + "\n";
        }
    }
    return output;
}

std::string Simulation::root_zone_csv_header() const {
    return "timestamp_s,root_zone_id,type,conservation_initial_storage_m3,current_storage_m3,capacity_m3,substrate_bulk_volume_m3,vwc,storage_fraction,"
           "cumulative_irrigation_topoff_m3,cumulative_external_return_m3,cumulative_realized_withdrawal_m3,cumulative_drainage_discharge_m3,"
           "cumulative_evaporation_m3,cumulative_unmet_demand_m3,conservation_residual_m3\n";
}
std::string Simulation::root_zone_csv_row() const {
    std::string output;
    for (const rootzone::RootZoneState& rz : state_.root_zones) {
        std::string type_str = (rz.type == rootzone::RootZoneType::Substrate) ? "Substrate" : "Reservoir";
        std::string vwc_str = rz.volumetric_water_content.has_value() ? std::to_string(*rz.volumetric_water_content) : "";
        std::string frac_str = rz.storage_fraction.has_value() ? std::to_string(*rz.storage_fraction) : "";

        double initial_water = rz.interval_start_water_volume.value;
        
        double residual = rz.current_water_volume.value - (
            initial_water + 
            rz.cumulative_irrigation_top_off.value + 
            rz.cumulative_external_return_flow.value - 
            rz.cumulative_realized_withdrawal.value - 
            rz.cumulative_drainage_discharge.value - 
            rz.cumulative_evaporation.value
        );

        output += std::to_string(state_.clock.elapsed.value) + "," + rz.id + "," + type_str + ",";
        output += std::to_string(initial_water) + ",";
        output += std::to_string(rz.current_water_volume.value) + ",";
        output += rz.max_stored_water.has_value() ? std::to_string(rz.max_stored_water->value) : "";
        output += "," + std::to_string(rz.substrate_bulk_volume.value) + ",";
        output += vwc_str + "," + frac_str + ",";
        output += std::to_string(rz.cumulative_irrigation_top_off.value) + ",";
        output += std::to_string(rz.cumulative_external_return_flow.value) + ",";
        output += std::to_string(rz.cumulative_realized_withdrawal.value) + ",";
        output += std::to_string(rz.cumulative_drainage_discharge.value) + ",";
        output += std::to_string(rz.cumulative_evaporation.value) + ",";
        output += std::to_string(rz.cumulative_unmet_demand.value) + ",";
        output += std::to_string(residual) + "\n";
    }
    return output;
}

std::string Simulation::plant_physiology_csv_header() const {
    return "timestamp_s,plant_id,room_id,x,y,z,cell_id,root_zone_id,ppfd,pressure,airflow,leaf_temperature,leaf_vpd,vwc,matric_potential,hydraulic_status,beta,g1_reference,g1_effective,assimilation,gs,gb,transpiration_flux,effective_leaf_area,requested_water,realized_water,unmet_demand,gas_exchange_status,transpiration_status,leaf_temperature_mode,leaf_temperature_source,incident_shortwave_w_m2,radiative_surroundings_temperature_c,shortwave_absorptance,longwave_emissivity,surface_factor,area_basis,absorbed_shortwave_w_m2,net_longwave_w_m2,sensible_heat_flux_w_m2,latent_heat_flux_w_m2,energy_residual_w_m2,leaf_energy_iterations,leaf_energy_status,leaf_energy_model_version,p2_model_version,consequence_stage,stage_progress_0_1,stage_opportunity_accumulated_s,stage_elapsed_s,potential_opportunity_s,achieved_opportunity_s,lost_opportunity_s,opportunity_signal_0_1,water_current_severity_0_1,thermal_current_severity_0_1,radiation_current_severity_0_1,water_exposure_dose_s,thermal_exposure_dose_s,radiation_exposure_dose_s,water_recovery_0_1,thermal_recovery_0_1,radiation_recovery_0_1,water_damage_0_1,thermal_damage_0_1,radiation_damage_0_1,size_proxy_0_1,vigor_signal_0_1,wilt_severity_0_1,growth_slowdown_0_1,heat_stress_severity_0_1,radiation_stress_severity_0_1,damage_severity_0_1,recovery_severity_0_1,derived_condition,consequence_status,opportunity_available,water_consequence_available,thermal_consequence_available,radiation_consequence_available,reason_codes\n";
}

std::string Simulation::plant_physiology_csv_row() const {
    std::string output;
    for (const plants::PlantState& plant : state_.plants) {
        const auto& ge = plant.latent.gas_exchange;
        const auto& tr = plant.latent.transpiration;
        
        std::string ppfd = "", press = "", airf = "", ltemp = "", lvpd = "";
        std::string shortwave = "", radiative = "";
        std::string vwc = "", mat = "", hstat = "";
        std::string c_id = "";
        
        for (const auto& room : state_.rooms) {
            if (room.id == plant.location.room_id) {
                if (!room.cells.empty()) {
                    auto idx = cell_index_for(room, plant.location.x, plant.location.y);
                    if (idx < room.cells.size()) {
                        const auto& cell = room.cells[idx];
                        c_id = cell.id;
                        const auto& env = cell.environment;
                        ppfd = std::to_string(cell.lighting.ppfd.value);
                        press = std::to_string(env.atmospheric_pressure.value);
                        if (env.airflow) airf = std::to_string(env.airflow->value);
                        if (plant.latent.leaf_energy.leaf_temperature) ltemp = std::to_string(plant.latent.leaf_energy.leaf_temperature->value);
                        else if (env.leaf_temperature) ltemp = std::to_string(env.leaf_temperature->value);
                        if (plant.latent.leaf_energy.leaf_vpd) lvpd = std::to_string(plant.latent.leaf_energy.leaf_vpd->value);
                        else if (env.leaf_vpd) lvpd = std::to_string(env.leaf_vpd->value);
                        if (cell.lighting.incident_shortwave_w_m2) shortwave = std::to_string(*cell.lighting.incident_shortwave_w_m2);
                        else if (env.incident_shortwave_w_m2) shortwave = std::to_string(*env.incident_shortwave_w_m2);
                        if (env.radiative_surroundings_temperature) radiative = std::to_string(env.radiative_surroundings_temperature->value);
                    }
                }
            }
        }
        
        for (const auto& rz : state_.root_zones) {
            if (rz.id == plant.root_zone_id) {
                if (rz.volumetric_water_content) vwc = std::to_string(*rz.volumetric_water_content);
                if (rz.matric_potential) mat = std::to_string(*rz.matric_potential);
                if (rz.hydraulic_status) hstat = *rz.hydraulic_status;
            }
        }

        const bool gas_exchange_available = (ge.status == gasexchange::ConvergenceStatus::Converged);
        const bool transpiration_available = gas_exchange_available && tr.available;
        
        output += std::to_string(state_.clock.elapsed.value) + ",";
        output += plant.id + "," + plant.location.room_id + ",";
        output += std::to_string(plant.location.x.value) + "," + std::to_string(plant.location.y.value) + "," + std::to_string(plant.location.z.value) + ",";
        output += c_id + "," + plant.root_zone_id + ",";
        output += ppfd + "," + press + "," + airf + "," + ltemp + "," + lvpd + ",";
        output += vwc + "," + mat + "," + hstat + ",";
        output += (ge.beta_hydraulic ? std::to_string(*ge.beta_hydraulic) : "") + ",";
        const bool gas_profile_available = !ge.profile_id.empty() && ge.profile_id != "unconfigured";
        output += (gas_profile_available ? std::to_string(ge.g1_reference) : "") + ",";
        output += (gas_exchange_available ? std::to_string(ge.g1_effective) : "") + ",";
        output += (gas_exchange_available ? std::to_string(ge.net_assimilation.value) : "") + ",";
        output += (gas_exchange_available ? std::to_string(ge.stomatal_conductance.value) : "") + ",";
        output += (transpiration_available && plant.latent.boundary_layer.value_mol_m2_s ? std::to_string(*plant.latent.boundary_layer.value_mol_m2_s) : "") + ",";
        output += (transpiration_available ? std::to_string(tr.flux_mol_m2_s) : "") + ",";
        output += (transpiration_available && plant.latent.effective_leaf_area_m2 ? std::to_string(*plant.latent.effective_leaf_area_m2) : "") + ",";
        output += (transpiration_available ? std::to_string(plant.latent.requested_water_mol) : "") + ",";
        output += (transpiration_available ? std::to_string(plant.latent.realized_water_mol) : "") + ",";
        output += (transpiration_available ? std::to_string(plant.latent.unmet_demand_mol) : "") + ",";
        output += gasexchange::to_string(ge.status) + ",";
        output += tr.status + ",";
        output += leafenergy::to_string(plant.leaf_energy_profile.temperature_mode) + ",";
        output += plant.latent.leaf_energy.source + ",";
        output += shortwave + "," + radiative + ",";
        const bool profile_available = plant.leaf_energy_profile.is_configured;
        output += (profile_available ? std::to_string(plant.leaf_energy_profile.shortwave_absorptance) : "");
        output += ",";
        output += (profile_available ? std::to_string(plant.leaf_energy_profile.longwave_emissivity) : "");
        output += ",";
        output += (profile_available ? std::to_string(plant.leaf_energy_profile.surface_factor) : "");
        output += "," + (profile_available ? leafenergy::to_string(plant.leaf_energy_profile.area_basis) : "") + ",";
        const auto opt_string = [](const std::optional<double>& value) {
            return value.has_value() ? std::to_string(*value) : std::string{};
        };
        output += opt_string(plant.latent.leaf_energy.absorbed_shortwave_w_m2) + ",";
        output += opt_string(plant.latent.leaf_energy.net_longwave_w_m2) + ",";
        output += opt_string(plant.latent.leaf_energy.sensible_heat_flux_w_m2) + ",";
        output += opt_string(plant.latent.leaf_energy.latent_heat_flux_w_m2) + ",";
        output += opt_string(plant.latent.leaf_energy.energy_residual_w_m2) + ",";
        output += std::to_string(plant.latent.leaf_energy.solver_iterations) + ",";
        output += plant.latent.leaf_energy.status + ",";
        output += plant.latent.leaf_energy.model_version + ",";
        const auto& consequence = plant.consequence_state;
        const auto& consequence_view = plant.consequence_presentation;
        const bool p2_configured = plant.consequence_profile.is_configured;
        const auto p2_number = [p2_configured](double value) {
            return p2_configured ? std::to_string(value) : std::string{};
        };
        output += p2_configured ? consequence.model_version : "";
        output += "," + (p2_configured ? consequences::to_string(consequence.stage) : "");
        output += "," + p2_number(consequence.stage_progress_0_1);
        output += "," + p2_number(consequence.stage_opportunity_accumulated_s);
        output += "," + p2_number(consequence.stage_elapsed_s);
        output += "," + p2_number(consequence.potential_opportunity_s);
        output += "," + p2_number(consequence.achieved_opportunity_s);
        output += "," + p2_number(consequence.lost_opportunity_s);
        output += "," + p2_number(consequence.current_opportunity_signal_0_1);
        output += "," + p2_number(consequence.water_current_severity_0_1);
        output += "," + p2_number(consequence.thermal_current_severity_0_1);
        output += "," + p2_number(consequence.radiation_current_severity_0_1);
        output += "," + p2_number(consequence.water_exposure_dose_s);
        output += "," + p2_number(consequence.thermal_exposure_dose_s);
        output += "," + p2_number(consequence.radiation_exposure_dose_s);
        output += "," + p2_number(consequence.water_recovery_0_1);
        output += "," + p2_number(consequence.thermal_recovery_0_1);
        output += "," + p2_number(consequence.radiation_recovery_0_1);
        output += "," + p2_number(consequence.water_damage_0_1);
        output += "," + p2_number(consequence.thermal_damage_0_1);
        output += "," + p2_number(consequence.radiation_damage_0_1);
        output += "," + p2_number(consequence_view.size_proxy_0_1);
        output += "," + p2_number(consequence_view.vigor_signal_0_1);
        output += "," + p2_number(consequence_view.wilt_severity_0_1);
        output += "," + p2_number(consequence_view.growth_slowdown_0_1);
        output += "," + p2_number(consequence_view.heat_stress_severity_0_1);
        output += "," + p2_number(consequence_view.radiation_stress_severity_0_1);
        output += "," + p2_number(consequence_view.damage_severity_0_1);
        output += "," + p2_number(consequence_view.recovery_severity_0_1);
        output += "," + (p2_configured ? consequence_view.derived_condition : "");
        output += "," + (p2_configured ? consequence.integration_status : "profile_unconfigured");
        output += ",";
        output += p2_configured ? (consequence.opportunity_available ? "1" : "0") : "";
        output += ",";
        output += p2_configured ? (consequence.water_available ? "1" : "0") : "";
        output += ",";
        output += p2_configured ? (consequence.thermal_available ? "1" : "0") : "";
        output += ",";
        output += p2_configured ? (consequence.radiation_available ? "1" : "0") : "";
        std::string reason_codes;
        if (p2_configured) {
            for (std::size_t index = 0; index < consequence_view.reason_codes.size(); ++index) {
                if (index != 0) reason_codes += "|";
                reason_codes += consequence_view.reason_codes[index].reason_code;
            }
        }
        output += "," + reason_codes + "\n";
    }
    return output;
}

const std::vector<SegmentDiagnostic>& Simulation::last_step_segment_trace() const {
    return last_step_segment_trace_;
}

} // namespace cannaville::core
