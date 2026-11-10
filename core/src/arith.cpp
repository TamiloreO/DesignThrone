#include "vx/arith.hpp"

#include <limits>

namespace vx {
namespace {

constexpr std::uint64_t kHalfMask = 0xffffffffull;

// 64x64 -> 128 bit multiply using 32 bit limbs. No compiler extension.
void multiplyWide(std::uint64_t a, std::uint64_t b, std::uint64_t& high, std::uint64_t& low) {
    const std::uint64_t aLow = a & kHalfMask;
    const std::uint64_t aHigh = a >> 32;
    const std::uint64_t bLow = b & kHalfMask;
    const std::uint64_t bHigh = b >> 32;

    const std::uint64_t lowLow = aLow * bLow;
    const std::uint64_t lowHigh = aLow * bHigh;
    const std::uint64_t highLow = aHigh * bLow;
    const std::uint64_t highHigh = aHigh * bHigh;

    const std::uint64_t middle = (lowLow >> 32) + (lowHigh & kHalfMask) + (highLow & kHalfMask);
    low = (lowLow & kHalfMask) | (middle << 32);
    high = highHigh + (lowHigh >> 32) + (highLow >> 32) + (middle >> 32);
}

// 128 / 64 -> 64 bit division by binary long division. Requires high < divisor,
// which the caller checks, so the quotient is guaranteed to fit in 64 bits.
std::uint64_t divideWide(std::uint64_t high, std::uint64_t low, std::uint64_t divisor,
                         std::uint64_t& remainder) {
    std::uint64_t quotient = 0;
    std::uint64_t rem = high;
    for (int bit = 63; bit >= 0; --bit) {
        const std::uint64_t carry = rem >> 63;
        rem = (rem << 1) | ((low >> bit) & 1ull);
        quotient <<= 1;
        if (carry != 0 || rem >= divisor) {
            rem -= divisor;
            quotient |= 1ull;
        }
    }
    remainder = rem;
    return quotient;
}

std::uint64_t magnitude(std::int64_t value) {
    // Written this way so that INT64_MIN, which has no positive counterpart,
    // converts without signed overflow.
    return value < 0 ? static_cast<std::uint64_t>(-(value + 1)) + 1ull
                     : static_cast<std::uint64_t>(value);
}

}  // namespace

std::int64_t mulDiv(std::int64_t value, std::int64_t numerator, std::int64_t denominator) {
    if (denominator <= 0) return 0;
    if (numerator == 0 || value == 0) return 0;

    const bool negative = (value < 0) != (numerator < 0);
    const std::uint64_t valueMagnitude = magnitude(value);
    const std::uint64_t numeratorMagnitude = magnitude(numerator);

    std::uint64_t high = 0;
    std::uint64_t low = 0;
    multiplyWide(valueMagnitude, numeratorMagnitude, high, low);

    const auto divisor = static_cast<std::uint64_t>(denominator);
    if (high >= divisor) {
        return negative ? std::numeric_limits<std::int64_t>::min()
                        : std::numeric_limits<std::int64_t>::max();
    }

    std::uint64_t remainder = 0;
    const std::uint64_t quotient = divideWide(high, low, divisor, remainder);

    constexpr std::uint64_t kInt64Max = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if (negative) {
        if (quotient > kInt64Max + 1ull) return std::numeric_limits<std::int64_t>::min();
        if (quotient == kInt64Max + 1ull) return std::numeric_limits<std::int64_t>::min();
        return -static_cast<std::int64_t>(quotient);
    }
    if (quotient > kInt64Max) return std::numeric_limits<std::int64_t>::max();
    return static_cast<std::int64_t>(quotient);
}

}  // namespace vx
