#pragma once

#include "Simulation/Core/Units.hpp"
#include "Simulation/Environment/EnvironmentState.hpp"
#include "Simulation/Lighting/LightingState.hpp"
#include "Simulation/Plants/PlantState.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace cannaville::core {

struct SimulationClock {
    units::Seconds elapsed;
    units::SimulationStepCount steps;
};

struct RoomCellState {
    std::string id;
    units::Meters center_x;
    units::Meters center_y;
    environment::EnvironmentState environment;
    lighting::LightingState lighting;
};

struct RoomState {
    std::string id;
    units::Meters width;
    units::Meters depth;
    units::Meters cell_size;
    std::vector<RoomCellState> cells;
};

struct SimulationStochasticState {
    std::uint64_t seed{0};
    std::uint64_t rng_state{0};
    std::uint64_t draws_consumed{0};
};

struct SimulationConfig {
    std::string simulation_version;
    units::Seconds fixed_timestep;
};

struct SimulationState {
    SimulationConfig config;
    std::string scenario_id;
    SimulationClock clock;
    SimulationStochasticState stochastic;
    std::vector<RoomState> rooms;
    std::vector<plants::PlantState> plants;
};

struct ObservablePlantState {
    std::string id;
    std::string room_id;
    units::Meters x;
    units::Meters y;
    units::Meters z;
    plants::GrowthStage displayed_growth_stage{plants::GrowthStage::Seedling};
    bool interaction_available{false};
    std::string readiness_label;
};

struct ObservableSimulationState {
    std::string simulation_version;
    std::string scenario_id;
    SimulationClock clock;
    std::vector<RoomState> rooms;
    std::vector<ObservablePlantState> plants;
};

} // namespace cannaville::core
