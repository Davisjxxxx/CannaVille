#pragma once

#include "Simulation/Core/DeterministicRng.hpp"
#include "Simulation/Core/Scenario.hpp"
#include "Simulation/Core/SimulationState.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace cannaville::core {

struct PlayerAction {
    std::string action_id;
    std::string target_id;
    std::string payload_json{"{}"};
};

struct OfflineReconciliation {
    units::Seconds requested;
    units::Seconds advanced;
    units::Seconds remainder;
};

class ISimulation {
public:
    virtual ~ISimulation() = default;
    virtual void initialize(const Scenario& scenario, std::uint64_t seed) = 0;
    virtual void submit_player_action(const PlayerAction& action) = 0;
    virtual void advance_fixed_step() = 0;
    virtual OfflineReconciliation reconcile_offline_elapsed(units::Seconds elapsed) = 0;
    virtual ObservableSimulationState observable_state() const = 0;
    virtual std::string serialize_state() const = 0;
    virtual void load_serialized_state(std::string_view serialized) = 0;
};

class Simulation final : public ISimulation {
public:
    Simulation() = default;
    Simulation(const Scenario& scenario, std::uint64_t seed);

    void initialize(const Scenario& scenario, std::uint64_t seed) override;
    void submit_player_action(const PlayerAction& action) override;
    void advance_fixed_step() override;
    OfflineReconciliation reconcile_offline_elapsed(units::Seconds elapsed) override;
    ObservableSimulationState observable_state() const override;
    std::string serialize_state() const override;
    void load_serialized_state(std::string_view serialized) override;

    const Scenario& scenario() const;
    const SimulationState& full_state_for_internal_use() const;
    std::uint64_t seed() const;
    std::string csv_header() const;
    std::string csv_row() const;

private:
    Scenario scenario_;
    SimulationState state_;
    DeterministicRng rng_;
};

} // namespace cannaville::core
