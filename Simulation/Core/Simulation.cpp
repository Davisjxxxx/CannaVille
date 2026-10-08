#include "Simulation/Core/Simulation.hpp"

#include "Simulation/Core/DeterministicRng.hpp"
#include "Simulation/Core/Serialization.hpp"

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

    for (const ScenarioRoomDefinition& definition : scenario.rooms) {
        RoomState room;
        room.id = definition.id;
        room.width = definition.width;
        room.depth = definition.depth;
        room.cell_size = definition.cell_size;
        const std::size_t x_count = static_cast<std::size_t>(std::ceil(room.width.value / room.cell_size.value));
        const std::size_t y_count = static_cast<std::size_t>(std::ceil(room.depth.value / room.cell_size.value));
        room.cells.reserve(x_count * y_count);
        for (std::size_t y = 0; y < y_count; ++y) {
            for (std::size_t x = 0; x < x_count; ++x) {
                RoomCellState cell;
                cell.id = room.id + "/cell-" + std::to_string(x) + "-" + std::to_string(y);
                cell.center_x.value = (static_cast<double>(x) + 0.5) * room.cell_size.value;
                cell.center_y.value = (static_cast<double>(y) + 0.5) * room.cell_size.value;
                room.cells.push_back(std::move(cell));
            }
        }
        state_.rooms.push_back(std::move(room));
    }

    for (const ScenarioPlantDefinition& definition : scenario.plants) {
        plants::PlantState plant;
        plant.id = definition.id;
        plant.location.room_id = definition.room_id;
        plant.location.x = definition.x;
        plant.location.y = definition.y;
        plant.location.z = definition.z;
        plant.genetics.cultivar_id = definition.cultivar_id;
        plant.stochastic.stream_seed = seed;
        plant.stochastic.stream_state = seed;
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
    const double timestep = state_.config.fixed_timestep.value;
    if (!(timestep > 0.0) || !std::isfinite(timestep)) {
        throw std::logic_error("simulation has no valid fixed timestep");
    }
    state_.clock.elapsed.value += timestep;
    ++state_.clock.steps.value;
    for (plants::PlantState& plant : state_.plants) {
        plant.history.elapsed.value += timestep;
        ++plant.history.steps.value;
        const auto room_it = std::find_if(state_.rooms.begin(), state_.rooms.end(), [&](const RoomState& room) {
            return room.id == plant.location.room_id;
        });
        if (room_it != state_.rooms.end()) sample_plant_from_room(plant, *room_it);
    }
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
    scenario_.simulation_version = state_.config.simulation_version;
    scenario_.fixed_timestep = state_.config.fixed_timestep;
    for (const RoomState& room : state_.rooms) {
        scenario_.rooms.push_back(ScenarioRoomDefinition{room.id, room.width, room.depth, room.cell_size});
    }
    for (const plants::PlantState& plant : state_.plants) {
        scenario_.plants.push_back(ScenarioPlantDefinition{
            plant.id,
            plant.location.room_id,
            plant.genetics.cultivar_id,
            plant.location.x,
            plant.location.y,
            plant.location.z,
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
    return "simulation_version,scenario_id,seed,step_index,elapsed_seconds,room_count,plant_count\n";
}

std::string Simulation::csv_row() const {
    return state_.config.simulation_version + "," + state_.scenario_id + "," +
        std::to_string(state_.stochastic.seed) + "," +
        std::to_string(state_.clock.steps.value) + "," +
        std::to_string(state_.clock.elapsed.value) + "," +
        std::to_string(state_.rooms.size()) + "," +
        std::to_string(state_.plants.size()) + "\n";
}

} // namespace cannaville::core
