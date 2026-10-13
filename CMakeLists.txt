cmake_minimum_required(VERSION 3.20)

project(CannaVilleV2 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cannaville_sim
    Simulation/Core/Json.cpp
    Simulation/Core/Scenario.cpp
    Simulation/Core/Serialization.cpp
    Simulation/Core/Simulation.cpp
    Simulation/Environment/VaporPressure.cpp
    Simulation/Lighting/LightingSystem.cpp
    Simulation/GasExchange/GasExchange.cpp
    Simulation/Transpiration/Transpiration.cpp
    Simulation/LeafEnergy/LeafEnergy.cpp
    Simulation/RootZone/RootZoneWater.cpp
    Simulation/RootZone/HydraulicLimitation.cpp
    Simulation/Consequences/ConsequenceState.cpp
)

target_include_directories(cannaville_sim PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if (MSVC)
    target_compile_options(cannaville_sim PRIVATE /W4 /permissive-)
else()
    target_compile_options(cannaville_sim PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(cannaville-sim Tools/SimCLI/main.cpp)
target_link_libraries(cannaville-sim PRIVATE cannaville_sim)

add_executable(cannaville-tests Tests/SimulationTests.cpp)
target_link_libraries(cannaville-tests PRIVATE cannaville_sim)

add_executable(cannaville-p1b5-tests Tests/LeafEnergyTests.cpp)
target_link_libraries(cannaville-p1b5-tests PRIVATE cannaville_sim)

add_executable(cannaville-p1b5-benchmark Tools/P1B5Benchmark.cpp)
target_link_libraries(cannaville-p1b5-benchmark PRIVATE cannaville_sim)

add_executable(cannaville-p2-tests Tests/P2Tests.cpp)
target_link_libraries(cannaville-p2-tests PRIVATE cannaville_sim)

add_executable(cannaville-p2-benchmark Tools/P2Benchmark.cpp)
target_link_libraries(cannaville-p2-benchmark PRIVATE cannaville_sim)

enable_testing()
add_test(NAME cannaville-tests COMMAND cannaville-tests)
add_test(NAME cannaville-p1b5-tests COMMAND cannaville-p1b5-tests)
add_test(NAME cannaville-p2-tests COMMAND cannaville-p2-tests)
