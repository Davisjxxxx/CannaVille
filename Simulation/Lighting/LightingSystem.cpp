#include "Simulation/Lighting/LightingSystem.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace cannaville::lighting {

namespace {

struct LightSample {
    double ppfd{0.0};
    bool light_on{false};
};

LightSample sample_at(const LightingSchedule& schedule, double seconds_of_day) {
    for (const LightingScheduleSegment& segment : schedule.segments) {
        if (seconds_of_day >= segment.start_of_day.value && seconds_of_day < segment.end_of_day.value) {
            return LightSample{segment.ppfd.value, segment.ppfd.value > 0.0};
        }
    }
    return LightSample{};
}

double next_schedule_boundary(const LightingSchedule& schedule, double seconds_of_day) {
    double boundary = kSimulationDaySeconds;
    for (const LightingScheduleSegment& segment : schedule.segments) {
        if (segment.start_of_day.value > seconds_of_day) boundary = std::min(boundary, segment.start_of_day.value);
        if (segment.end_of_day.value > seconds_of_day) boundary = std::min(boundary, segment.end_of_day.value);
    }
    return boundary;
}

void reset_day(LightingState& state, const LightingSchedule& schedule) {
    state.dli.value = 0.0;
    state.accumulated_light_on_duration.value = 0.0;
    state.accumulated_dark_duration.value = 0.0;
    state.photoperiod_duration.value = 0.0;
    for (const LightingScheduleSegment& segment : schedule.segments) {
        if (segment.ppfd.value > 0.0) {
            state.photoperiod_duration.value += segment.end_of_day.value - segment.start_of_day.value;
        }
    }
}

} // namespace

std::vector<std::string> validate_schedule(const LightingSchedule& schedule) {
    std::vector<std::string> errors;
    for (std::size_t index = 0; index < schedule.segments.size(); ++index) {
        const LightingScheduleSegment& segment = schedule.segments[index];
        if (!std::isfinite(segment.start_of_day.value) || !std::isfinite(segment.end_of_day.value) ||
            segment.start_of_day.value < 0.0 || segment.end_of_day.value > kSimulationDaySeconds ||
            segment.start_of_day.value >= segment.end_of_day.value) {
            errors.push_back("lighting schedule segments must satisfy 0 <= start < end <= 86400 seconds");
        }
        if (!std::isfinite(segment.ppfd.value) || segment.ppfd.value < 0.0) {
            errors.push_back("lighting schedule PPFD must be finite and non-negative");
        }
        for (std::size_t previous_index = 0; previous_index < index; ++previous_index) {
            const LightingScheduleSegment& previous = schedule.segments[previous_index];
            if (segment.start_of_day.value < previous.end_of_day.value &&
                previous.start_of_day.value < segment.end_of_day.value) {
                errors.push_back("lighting schedule segments must not overlap");
            }
        }
    }
    return errors;
}

void initialize_state(LightingState& state, const LightingSchedule& schedule, double simulation_seconds) {
    const std::vector<std::string> errors = validate_schedule(schedule);
    if (!errors.empty()) throw std::invalid_argument(errors.front());
    state = LightingState{};
    reset_day(state, schedule);
    const double seconds_of_day = std::fmod(std::max(0.0, simulation_seconds), kSimulationDaySeconds);
    const LightSample sample = sample_at(schedule, seconds_of_day);
    state.ppfd.value = sample.ppfd;
    state.light_on = sample.light_on;
}

void advance_state(LightingState& state,
                   const LightingSchedule& schedule,
                   double simulation_seconds,
                   double duration_seconds) {
    if (!std::isfinite(simulation_seconds) || !std::isfinite(duration_seconds) || duration_seconds < 0.0) {
        throw std::invalid_argument("lighting advance requires finite non-negative duration");
    }
    const std::vector<std::string> errors = validate_schedule(schedule);
    if (!errors.empty()) throw std::invalid_argument(errors.front());

    double remaining = duration_seconds;
    double cursor = simulation_seconds;
    while (remaining > 0.0) {
        double seconds_of_day = std::fmod(std::max(0.0, cursor), kSimulationDaySeconds);
        if (seconds_of_day == 0.0 && cursor > 0.0) reset_day(state, schedule);
        const double boundary = next_schedule_boundary(schedule, seconds_of_day);
        const double segment_duration = std::min(remaining, boundary - seconds_of_day);
        if (!(segment_duration > 0.0)) {
            cursor += 1e-9;
            continue;
        }
        const LightSample sample = sample_at(schedule, seconds_of_day + 1e-9);
        state.ppfd.value = sample.ppfd;
        state.light_on = sample.light_on;
        state.dli.value += sample.ppfd * segment_duration / 1000000.0;
        if (sample.light_on) {
            state.accumulated_light_on_duration.value += segment_duration;
        } else {
            state.accumulated_dark_duration.value += segment_duration;
        }
        cursor += segment_duration;
        remaining -= segment_duration;
        if (std::abs(std::fmod(cursor, kSimulationDaySeconds)) < 1e-9) {
            reset_day(state, schedule);
        }
    }
    const double final_seconds_of_day = std::fmod(std::max(0.0, cursor), kSimulationDaySeconds);
    const LightSample final_sample = sample_at(schedule, final_seconds_of_day);
    state.ppfd.value = final_sample.ppfd;
    state.light_on = final_sample.light_on;
}

} // namespace cannaville::lighting
