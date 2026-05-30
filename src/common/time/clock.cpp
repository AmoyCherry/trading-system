#include "time/clock.hpp"
#include <ctime>
#include <chrono>
#include <format>

namespace ts::time {

std::uint64_t now_ns() noexcept {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1'000'000'000ull
         + static_cast<std::uint64_t>(ts.tv_nsec);
}
} // namespace ts::time
