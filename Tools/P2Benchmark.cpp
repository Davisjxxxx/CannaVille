#include "Simulation/Consequences/ConsequenceState.hpp"

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

using cannaville::consequences::PlantConsequencePresentation;
using cannaville::consequences::PlantConsequenceProfile;
using cannaville::consequences::PlantConsequenceState;
using cannaville::consequences::SegmentInputs;

PlantConsequenceProfile benchmark_profile() {
    auto profile = cannaville::consequences::get_synthetic_test_profile();
    for (auto& stage : profile.stages) {
        stage.required_opportunity_s = 1.0e12;
        stage.assimilation_reference_umol_m2_s = 10.0;
    }
    profile.source = "P2 deterministic performance fixture";
    profile.valid_domain = "benchmark only";
    return profile;
}

void run_case(std::size_t plant_count, const PlantConsequenceProfile& profile) {
    constexpr std::size_t segment_count = 100;
    std::vector<PlantConsequenceState> states(plant_count);
    std::vector<PlantConsequencePresentation> presentations(plant_count);
    SegmentInputs inputs;
    inputs.duration_s = 60.0;
    inputs.net_assimilation_umol_m2_s = 10.0;
    inputs.hydraulic_beta = 1.0;
    inputs.water_allocation_available = true;
    inputs.requested_water_mol = 1.0;
    inputs.realized_water_mol = 1.0;
    inputs.leaf_temperature_c = 24.0;
    inputs.absorbed_shortwave_w_m2 = 100.0;
    inputs.gas_exchange_status = "Converged";
    inputs.transpiration_status = "Valid";

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t segment = 0; segment < segment_count; ++segment) {
        inputs.start_time_s = static_cast<double>(segment) * inputs.duration_s;
        for (std::size_t plant = 0; plant < plant_count; ++plant) {
            cannaville::consequences::integrate_segment(states[plant], presentations[plant], profile, inputs);
        }
    }
    const auto finish = std::chrono::steady_clock::now();
    const double elapsed_ms = std::chrono::duration<double, std::milli>(finish - start).count();
    double checksum = 0.0;
    for (const auto& state : states) checksum += state.achieved_opportunity_s + state.water_exposure_dose_s;
    const double per_plant_segment_us = elapsed_ms * 1000.0 /
        static_cast<double>(plant_count * segment_count);
    std::cout << std::fixed << std::setprecision(3)
              << "P2_BENCHMARK plants=" << plant_count
              << " segments=" << segment_count
              << " elapsed_ms=" << elapsed_ms
              << " per_plant_segment_us=" << per_plant_segment_us
              << " checksum=" << checksum << "\n";
}

} // namespace

int main() {
    const auto profile = benchmark_profile();
    for (const std::size_t plant_count : {std::size_t{10}, std::size_t{100}, std::size_t{1000}, std::size_t{10000}}) {
        run_case(plant_count, profile);
    }
    return 0;
}
