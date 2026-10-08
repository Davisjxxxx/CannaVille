#pragma once

#include <cstdint>

namespace cannaville::units {

// SI-oriented, named quantities. These wrappers intentionally prevent a bare
// biological number from entering an interface without a unit-bearing name.
struct Seconds {
    double value{};
};

struct Meters {
    double value{};
};

struct Celsius {
    double value{};
};

struct RelativeHumidityFraction {
    double value{};
};

struct RelativeHumidityPercent {
    double value{};
};

struct AtmosphericPressureKPa {
    double value{};
};

struct VaporPressureKPa {
    double value{};
};

struct VPDKPa {
    double value{};
};

struct CO2MicromolesPerMole {
    double value{};
};

struct AirflowMetersPerSecond {
    double value{};
};

struct PPFDMicromolesPerSquareMeterSecond {
    double value{};
};

struct DLIMolesPerSquareMeterPerDay {
    double value{};
};

struct PhotoperiodHours {
    double value{};
};

struct PH {
    double value{};
};

struct EcmilliSiemensPerCentimeter {
    double value{};
};

struct SubstrateMoistureFraction {
    double value{};
};

struct MassGrams {
    double value{};
};

struct TemperatureCelsius {
    double value{};
};

struct AssimilationMicromolesPerSquareMeterSecond {
    double value{};
};

struct StomatalConductanceMolesPerSquareMeterSecond {
    double value{};
};

struct IntercellularCO2MicromolesPerMole {
    double value{};
};

struct SimulationStepCount {
    std::uint64_t value{};
};

} // namespace cannaville::units
