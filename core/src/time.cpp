#include "vx/time.hpp"

#include <cstdio>

namespace vx {

std::string timecodeString(const Time& time, const FrameRate& rate) {
    const auto frame = frameDuration(rate);
    const auto frameNs = frame.ns > 0 ? frame.ns : 1;

    std::int64_t totalFrames = time.ns / frameNs;
    const auto framesPerSecond = static_cast<std::int64_t>(rate.numerator) / rate.denominator;

    const auto fps = framesPerSecond > 0 ? framesPerSecond : 1;
    const auto ff = totalFrames % fps;
    const auto totalSeconds = totalFrames / fps;
    const auto ss = totalSeconds % 60;
    const auto mm = (totalSeconds / 60) % 60;
    const auto hh = totalSeconds / 3600;

    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%02lld:%02lld:%02lld:%02lld",
                  static_cast<long long>(hh), static_cast<long long>(mm),
                  static_cast<long long>(ss), static_cast<long long>(ff));
    return std::string(buffer);
}

}  // namespace vx
