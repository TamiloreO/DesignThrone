#pragma once

#include <cstdint>

namespace vx {

// Exact (value * numerator) / denominator, truncated toward zero.
//
// Speed changes need a product that is wider than 64 bits: a nanosecond time
// multiplied by a speed numerator easily exceeds INT64_MAX. GCC and Clang offer
// __int128 for this, but MSVC does not, and the Windows build has to produce
// identical results. So the wide arithmetic is emulated here with 64 bit halves
// instead of relying on a compiler extension.
//
// denominator must be positive. Results that do not fit in 64 bits saturate
// rather than wrap, because wrapping a timeline position silently corrupts a
// project and saturating makes the bug visible.
std::int64_t mulDiv(std::int64_t value, std::int64_t numerator, std::int64_t denominator);

}  // namespace vx
