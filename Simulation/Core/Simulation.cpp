#include <iostream>
#include "Simulation/Core/Simulation.hpp"

#include "Simulation/Core/DeterministicRng.hpp"
#include "Simulation/Core/Serialization.hpp"
#include "Simulation/GasExchange/GasExchange.hpp"
#include "Simulation/RootZone/HydraulicLimitation.hpp"
#include "Simulation/RootZone/RootZoneWater.hpp"
#include "Simulation/Transpiration/Transpiration.hpp"
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

        for (plants::PlantState& plant : state_.plants) {
            const auto room_it = std::find_if(state_.rooms.begin(), state_.rooms.end(), [&](const RoomState& room) {
                return room.id == plant.location.room_id;
            });
            if (room_it != state_.rooms.end()) sample_plant_from_room(plant, *room_it);

            gasexchange::ConvergenceStatus fail_status = gasexchange::ConvergenceStatus::NotRun;
            auto ge_profile = gasexchange::get_profile(plant.latent.gas_exchange.profile_id);
            
            std::optional<double> beta = std::nullopt;
            if (root_zone_betas.count(plant.root_zone_id)) {
                beta = root_zone_betas[plant.root_zone_id];
            }

            if (plant.latent.gas_exchange.profile_id.empty() || !ge_profile.is_configured) {
                fail_status = gasexchange::ConvergenceStatus::MissingGasExchangeProfile;
            } else if (!beta.has_value()) {
                // Determine why beta is missing
                fail_status = gasexchange::ConvergenceStatus::MissingHydraulicBeta;
                for (const auto& rz : state_.root_zones) {
                    if (rz.id == plant.root_zone_id) {
                        if (rz.type == rootzone::RootZoneType::Substrate) {
                            if (!rz.substrate_hydraulic_profile_id.has_value()) fail_status = gasexchange::ConvergenceStatus::MissingSubstrateHydraulicProfile;
                            else if (!rz.hydraulic_stress_transfer_profile_id.has_value()) fail_status = gasexchange::ConvergenceStatus::MissingHydraulicStressProfile;
                            else fail_status = gasexchange::ConvergenceStatus::HydraulicStateUnavailable;
                        } else {
                            if (!rz.explicit_unrestricted_water_access.value_or(false)) fail_status = gasexchange::ConvergenceStatus::HydraulicStateUnavailable;
                        }
                    }
                }
            } else if (!plant.sampled_environment.physics_available) {
                fail_status = gasexchange::ConvergenceStatus::MissingLeafTemperature; // proxy
            } else if (!plant.sampled_environment.leaf_temperature.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingLeafTemperature;
            } else if (!plant.sampled_environment.leaf_vpd.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingLeafVPD;
            } else if (!plant.sampled_environment.airflow.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingAirflow;
            } else if (!plant.latent.leaf_characteristic_dimension_m.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingLeafCharacteristicDimension;
            } else if (!plant.latent.effective_leaf_area_m2.has_value()) {
                fail_status = gasexchange::ConvergenceStatus::MissingEffectiveTranspiringLeafArea;
            }

            if (fail_status == gasexchange::ConvergenceStatus::NotRun) {
                units::PPFDMicromolesPerSquareMeterSecond ppfd = plant.sampled_lighting.ppfd;
                units::CO2MicromolesPerMole ambient_co2 = plant.sampled_environment.carbon_dioxide;
                units::VPDKPa vpd = *plant.sampled_environment.leaf_vpd;

                gasexchange::solve_coupled_gas_exchange(
                    plant.latent.gas_exchange,
                    ge_profile,
                    ppfd,
                    ambient_co2,
                    vpd,
                    plant.sampled_environment.leaf_temperature,
                    plant.sampled_environment.atmospheric_pressure,
                    beta
                );

                if (plant.latent.gas_exchange.status == gasexchange::ConvergenceStatus::Converged) {
                    auto air_props = transpiration::calculate_air_properties(
                        plant.sampled_environment.air_temperature,
                        plant.sampled_environment.atmospheric_pressure
                    );
                    
                    plant.latent.boundary_layer = transpiration::calculate_boundary_layer_conductance(
                        *plant.sampled_environment.airflow,
                        units::Meters{*plant.latent.leaf_characteristic_dimension_m},
                        air_props,
                        *plant.sampled_environment.leaf_temperature,
                        plant.sampled_environment.air_temperature
                    );

                    plant.latent.transpiration = transpiration::calculate_transpiration(
                        units::StomatalConductanceMolesPerSquareMeterSecond{plant.latent.gas_exchange.stomatal_conductance.value},
                        plant.latent.boundary_layer,
                        vpd,
                        plant.sampled_environment.atmospheric_pressure
                    );

                    double requested_mol = plant.latent.transpiration.flux_mol_m2_s * (*plant.latent.effective_leaf_area_m2) * segment_duration;
                    plant.latent.requested_water_mol = requested_mol;

                    rootzone::TranspirationRequest req;
                    req.flux = units::WaterFluxMolesPerSquareMeterSecond{plant.latent.transpiration.flux_mol_m2_s};
                    req.area = units::AreaSquareMeters{*plant.latent.effective_leaf_area_m2};
                    requests_by_rz[plant.root_zone_id].push_back(req);
                    plants_by_rz[plant.root_zone_id].push_back(&plant);
                } else {
                    plant.latent.transpiration.flux_mol_m2_s = 0.0;
                    plant.latent.requested_water_mol = 0.0;
                }
            } else {
                plant.latent.gas_exchange.status = fail_status;
                plant.latent.gas_exchange.net_assimilation.value = 0.0;
                plant.latent.gas_exchange.stomatal_conductance.value = 0.0;
                plant.latent.gas_exchange.intercellular_co2.value = 0.0;
                plant.latent.transpiration.flux_mol_m2_s = 0.0;
                plant.latent.requested_water_mol = 0.0;
            }
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
        });
    }
    return observable;
}

std::string Simulation::serialize_state() const {
    return serialize_state_json(state_);
}

void Simulation::load_serialized_state(std::string_view serialized) {
    state_ = deserialize_state_json(std::string(serialized));
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
            plant.latent.leaf_characteristic_dimension_m
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
           "accumulated_dark_duration_s,co2_umol_per_mol\n";
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
                std::to_string(cell.environment.carbon_dioxide.value) + "\n";
        }
    }
    return output;
}

std::string Simulation::root_zone_csv_header() const {
    return "timestamp_s,root_zone_id,type,substrate_bulk_volume_m3,interval_start_storage_m3,cumulative_irrigation_m3,cumulative_drainage_m3,cumulative_withdrawal_m3,interval_end_storage_m3,capacity_m3,conservation_residual_m3,"
           "vwc,storage_fraction,irrigation_topoff_m3,external_return_m3,"
           "realized_withdrawal_m3,drainage_discharge_m3,evaporation_m3,unmet_demand_m3,residual_m3\n";
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
        output += std::to_string(rz.substrate_bulk_volume.value) + ",";
        output += std::to_string(initial_water) + ",";
        output += std::to_string(rz.cumulative_irrigation_top_off.value) + ",";
        output += std::to_string(rz.cumulative_drainage_discharge.value) + ",";
        output += std::to_string(rz.cumulative_realized_withdrawal.value) + ",";
        output += std::to_string(rz.current_water_volume.value) + ",";
        output += rz.max_stored_water.has_value() ? std::to_string(rz.max_stored_water->value) : "";
        output += ",";
        output += std::to_string(residual) + ",";
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
    return "timestamp_s,plant_id,room_id,x,y,z,cell_id,root_zone_id,ppfd,pressure,airflow,leaf_temperature,leaf_vpd,vwc,matric_potential,hydraulic_status,beta,g1_reference,g1_effective,assimilation,gs,gb,transpiration_flux,effective_leaf_area,requested_water,realized_water,unmet_demand,gas_exchange_status,transpiration_status\n";
}

std::string Simulation::plant_physiology_csv_row() const {
    std::string output;
    for (const plants::PlantState& plant : state_.plants) {
        const auto& ge = plant.latent.gas_exchange;
        const auto& tr = plant.latent.transpiration;
        
        std::string ppfd = "", press = "", airf = "", ltemp = "", lvpd = "";
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
                        if (env.leaf_temperature) ltemp = std::to_string(env.leaf_temperature->value);
                        if (env.leaf_vpd) lvpd = std::to_string(env.leaf_vpd->value);
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

        bool ok = (ge.status == gasexchange::ConvergenceStatus::Converged);
        
        output += std::to_string(state_.clock.elapsed.value) + ",";
        output += plant.id + "," + plant.location.room_id + ",";
        output += std::to_string(plant.location.x.value) + "," + std::to_string(plant.location.y.value) + "," + std::to_string(plant.location.z.value) + ",";
        output += c_id + "," + plant.root_zone_id + ",";
        output += ppfd + "," + press + "," + airf + "," + ltemp + "," + lvpd + ",";
        output += vwc + "," + mat + "," + hstat + ",";
        output += (ge.beta_hydraulic ? std::to_string(*ge.beta_hydraulic) : "") + ",";
        output += ","; // g1 reference (omitted for now)
        output += (ok ? std::to_string(ge.g1_effective) : "") + ",";
        output += (ok ? std::to_string(ge.net_assimilation.value) : "") + ",";
        output += (ok ? std::to_string(ge.stomatal_conductance.value) : "") + ",";
        output += (ok && plant.latent.boundary_layer.value_mol_m2_s ? std::to_string(*plant.latent.boundary_layer.value_mol_m2_s) : "") + ",";
        output += (ok ? std::to_string(tr.flux_mol_m2_s) : "") + ",";
        output += (ok && plant.latent.effective_leaf_area_m2 ? std::to_string(*plant.latent.effective_leaf_area_m2) : "") + ",";
        output += (ok ? std::to_string(plant.latent.requested_water_mol) : "") + ",";
        output += (ok ? std::to_string(plant.latent.realized_water_mol) : "") + ",";
        output += (ok ? std::to_string(plant.latent.unmet_demand_mol) : "") + ",";
        output += std::to_string(static_cast<int>(ge.status)) + ",";
        output += tr.status + "\n";
    }
    return output;
}

} // namespace cannaville::core
