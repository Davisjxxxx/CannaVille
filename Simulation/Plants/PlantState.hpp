#pragma once

#include "Simulation/Core/Units.hpp"
#include "Simulation/Curing/CuringState.hpp"
#include "Simulation/Disease/DiseaseState.hpp"
#include "Simulation/Drying/DryingState.hpp"
#include "Simulation/Genetics/GeneticsState.hpp"
#include "Simulation/Harvest/HarvestState.hpp"
#include "Simulation/Lighting/LightingState.hpp"
#include "Simulation/Nutrition/NutritionState.hpp"
#include "Simulation/Pests/PestState.hpp"
#include "Simulation/RootZone/RootZoneState.hpp"
#include "Simulation/Treatments/TreatmentState.hpp"
#include "Simulation/GasExchange/GasExchange.hpp"

#include <cstdint>
#include <string>

#include "Simulation/Transpiration/Transpiration.hpp"

namespace cannaville::plants {

enum class GrowthStage {
    Seedling,
    Vegetative,
    Flowering,
    Harvest,
};

struct PlantLocation {
    std::string room_id;
    units::Meters x;
    units::Meters y;
    units::Meters z;
};


// Latent state is the eventual biological source of truth. Only identity and
// an explicit placeholder stage exist until governed models are added.
struct PlantLatentState {
    GrowthStage growth_stage{GrowthStage::Seedling};
    gasexchange::GasExchangeState gas_exchange;
    transpiration::TranspirationResult transpiration;
    transpiration::BoundaryLayerConductance boundary_layer;
    std::optional<double> effective_leaf_area_m2;
    std::optional<double> leaf_characteristic_dimension_m;
    double requested_water_mol{0.0};
    double realized_water_mol{0.0};
    double unmet_demand_mol{0.0};
};

struct PlantObservableState {
    GrowthStage displayed_growth_stage{GrowthStage::Seedling};
    bool interaction_available{true};
};

struct PlantDerivedState {
    std::string readiness_label{"not_modeled"};
};

struct PlantHistoryState {
    units::Seconds elapsed{};
    units::SimulationStepCount steps{};
};

struct PlantStochasticState {
    std::uint64_t stream_seed{0};
    std::uint64_t stream_state{0};
    std::uint64_t draws_consumed{0};
};

struct PlantState {
    std::string id;
    PlantLocation location;
    genetics::GeneticsState genetics;
    environment::EnvironmentState sampled_environment;
    lighting::LightingState sampled_lighting;
    std::string root_zone_id;
    nutrition::NutritionState nutrition;
    pests::PestPopulationState pests;
    disease::DiseaseState disease;
    treatments::TreatmentState treatment;
    harvest::HarvestState harvest;
    drying::DryingState drying;
    curing::CuringState curing;
    PlantLatentState latent;
    PlantObservableState observable;
    PlantDerivedState derived;
    PlantHistoryState history;
    PlantStochasticState stochastic;
};

} // namespace cannaville::plants
