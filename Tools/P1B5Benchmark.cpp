#include "Simulation/Environment/VaporPressure.hpp"
#include "Simulation/GasExchange/GasExchange.hpp"
#include "Simulation/LeafEnergy/LeafEnergy.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

namespace {

cannaville::environment::EnvironmentState make_environment() {
    cannaville::environment::EnvironmentState value;
    value.air_temperature.value = 25.0;
    value.relative_humidity.value = 50.0;
    value.atmospheric_pressure.value = 101.325;
    value.carbon_dioxide.value = 400.0;
    value.airflow = cannaville::units::AirflowMetersPerSecond{1.0};
    value.incident_shortwave_w_m2 = 500.0;
    value.radiative_surroundings_temperature = cannaville::units::Celsius{25.0};
    cannaville::environment::derive_physical_state(value);
    return value;
}

} // namespace

int main() {
    const auto env = make_environment();
    auto lighting = cannaville::lighting::LightingState{};
    lighting.ppfd.value = 1000.0;
    const auto profile = cannaville::leafenergy::get_synthetic_test_profile();
    const auto gas = cannaville::gasexchange::get_synthetic_vegetative_test_profile();
    for (const std::size_t count : {std::size_t{10}, std::size_t{100}, std::size_t{1000}, std::size_t{10000}}) {
        std::vector<std::uint32_t> iterations;
        iterations.reserve(count);
        std::size_t failures = 0;
        const auto start = std::chrono::steady_clock::now();
        for (std::size_t index = 0; index < count; ++index) {
            cannaville::leafenergy::LeafEnergyState state;
            cannaville::leafenergy::solve_leaf_energy_balance(
                state, profile, env, lighting, gas, 1.0, 0.1);
            iterations.push_back(state.solver_iterations);
            if (state.status != "converged") ++failures;
        }
        const auto stop = std::chrono::steady_clock::now();
        std::sort(iterations.begin(), iterations.end());
        const auto median = iterations[iterations.size() / 2];
        const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count();
        std::cout << "P1B5_BENCHMARK plants=" << count
                  << " median_iterations=" << median
                  << " max_iterations=" << iterations.back()
                  << " failures=" << failures
                  << " total_us=" << elapsed_us
                  << " per_plant_us=" << (static_cast<double>(elapsed_us) / static_cast<double>(count)) << "\n";
    }
    return 0;
}
