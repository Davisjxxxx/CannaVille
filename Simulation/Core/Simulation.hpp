#pragma once

#include "Simulation/Core/DeterministicRng.hpp"
#include "Simulation/Core/Scenario.hpp"
#include "Simulation/Core/SimulationState.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

// Engineering diagnostics for one deterministic integration segment. These
// values are intentionally not part of persisted simulation state; they prove
// event/light segmentation without becoming a second source of truth.
struct SegmentDiagnostic {
    std::string plant_id;
    std::string root_zone_id;
    double start_time_s{0.0};
    double duration_s{0.0};
    double ppfd_umol_m2_s{0.0};
    double water_input_before_segment_m3{0.0};
    double root_zone_storage_at_segment_start_m3{0.0};
    std::optional<double> vwc_at_segment_start;
    std::optional<double> matric_potential_mpa;
    std::optional<double> beta_hydraulic;
    double g1_effective{0.0};
    double stomatal_conductance_mol_m2_s{0.0};
    double net_assimilation_umol_m2_s{0.0};
    bool transpiration_available{false};
    std::optional<double> transpiration_flux_mol_m2_s;
    double requested_water_m3{0.0};
    double realized_water_m3{0.0};
    double post_segment_storage_m3{0.0};
    std::string gas_exchange_status;
    std::string transpiration_status;
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
    std::string root_zone_csv_header() const;
    std::string root_zone_csv_row() const;
    std::string plant_physiology_csv_header() const;
    std::string plant_physiology_csv_row() const;
    const std::vector<SegmentDiagnostic>& last_step_segment_trace() const;

private:
    Scenario scenario_;
    SimulationState state_;
    DeterministicRng rng_;
    std::vector<SegmentDiagnostic> last_step_segment_trace_;
};

} // namespace cannaville::core
