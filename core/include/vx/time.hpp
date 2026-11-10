#pragma once

#include <cstdint>
#include <string>

namespace vx {

// Time is stored as an exact integer number of nanoseconds.
//
// Floating point time is the single most common source of off-by-one-frame bugs
// in editors, so no part of this codebase is allowed to do time math in a double.
struct Time {
    std::int64_t ns = 0;

    static constexpr std::int64_t kNsPerSecond = 1'000'000'000;
    static constexpr std::int64_t kNsPerMs = 1'000'000;

    constexpr Time() = default;
    explicit constexpr Time(std::int64_t nanoseconds) : ns(nanoseconds) {}

    static constexpr Time fromNanoseconds(std::int64_t v) { return Time(v); }
    static constexpr Time fromMicroseconds(std::int64_t v) { return Time(v * 1'000); }
    static constexpr Time fromMilliseconds(std::int64_t v) { return Time(v * kNsPerMs); }
    static constexpr Time fromSeconds(std::int64_t v) { return Time(v * kNsPerSecond); }

    // Whole seconds only. Callers that need sub-second precision should build
    // the value with fromNanoseconds or with FrameRate.
    static constexpr Time fromSecondsDouble(double seconds) {
        const double scaled = seconds * static_cast<double>(kNsPerSecond);
        // Round half away from zero so negative values round symmetrically.
        return Time(static_cast<std::int64_t>(scaled < 0.0 ? scaled - 0.5 : scaled + 0.5));
    }

    constexpr double seconds() const { return static_cast<double>(ns) / static_cast<double>(kNsPerSecond); }

    constexpr bool operator==(const Time& other) const { return ns == other.ns; }
    constexpr bool operator!=(const Time& other) const { return ns != other.ns; }
    constexpr bool operator<(const Time& other) const { return ns < other.ns; }
    constexpr bool operator<=(const Time& other) const { return ns <= other.ns; }
    constexpr bool operator>(const Time& other) const { return ns > other.ns; }
    constexpr bool operator>=(const Time& other) const { return ns >= other.ns; }

    constexpr Time operator+(const Time& other) const { return Time(ns + other.ns); }
    constexpr Time operator-(const Time& other) const { return Time(ns - other.ns); }
    constexpr Time& operator+=(const Time& other) { ns += other.ns; return *this; }
    constexpr Time& operator-=(const Time& other) { ns -= other.ns; return *this; }

    constexpr Time operator*(std::int64_t scalar) const { return Time(ns * scalar); }

    constexpr bool isZero() const { return ns == 0; }
    constexpr bool isNegative() const { return ns < 0; }
    static constexpr Time zero() { return Time(0); }
    static constexpr Time max() { return Time(INT64_MAX); }
};

// A non-drop frame rate expressed exactly, as a fraction.
//
// 29.97 is 30000/1001, not 29.97. Storing it as a fraction is what keeps a
// two-hour timeline from drifting.
struct FrameRate {
    std::int32_t numerator = 30;
    std::int32_t denominator = 1;

    constexpr FrameRate() = default;
    constexpr FrameRate(std::int32_t num, std::int32_t den) : numerator(num), denominator(den) {}

    static constexpr FrameRate fps(std::int32_t whole) { return FrameRate(whole, 1); }

    constexpr double value() const {
        return static_cast<double>(numerator) / static_cast<double>(denominator);
    }

    constexpr bool operator==(const FrameRate& other) const {
        return numerator == other.numerator && denominator == other.denominator;
    }
    constexpr bool operator!=(const FrameRate& other) const { return !(*this == other); }

    constexpr bool isValid() const { return numerator > 0 && denominator > 0; }
};

// Exact duration of one frame, rounded to the nearest nanosecond.
inline Time frameDuration(const FrameRate& rate) {
    const auto n = static_cast<std::int64_t>(rate.denominator) * Time::kNsPerSecond;
    const auto d = static_cast<std::int64_t>(rate.numerator);
    return Time((n + d / 2) / d);
}

// Number of whole frames in a duration, rounded to nearest. Used for display
// and for snapping; never for accumulating time.
inline std::int64_t framesIn(const Time& time, const FrameRate& rate) {
    const auto n = static_cast<std::int64_t>(time.ns) * rate.numerator;
    const auto d = static_cast<std::int64_t>(rate.denominator) * Time::kNsPerSecond;
    const auto half = d / 2;
    return n >= 0 ? (n + half) / d : -((-n + half) / d);
}

// Time of a given frame index.
inline Time timeOfFrame(std::int64_t frame, const FrameRate& rate) {
    const auto n = frame * static_cast<std::int64_t>(rate.denominator) * Time::kNsPerSecond;
    const auto d = static_cast<std::int64_t>(rate.numerator);
    const auto half = d / 2;
    return Time(n >= 0 ? (n + half) / d : -((-n + half) / d));
}

// Snap a time to the nearest frame boundary.
inline Time snapToFrame(const Time& time, const FrameRate& rate) {
    return timeOfFrame(framesIn(time, rate), rate);
}

// Round a duration down to a whole number of frames, never below one frame.
inline Time floorToFrames(const Time& time, const FrameRate& rate) {
    const auto frame = frameDuration(rate);
    if (time.ns <= 0) return Time::zero();
    return timeOfFrame(static_cast<std::int64_t>(time.ns / frame.ns), rate);
}

// Human readable timecode, HH:MM:SS:FF.
std::string timecodeString(const Time& time, const FrameRate& rate);

}  // namespace vx
