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

std::string get_filename_with_ymds(const std::string& base) {
    auto now = std::chrono::system_clock::now();
    std::string suffix = std::format("{:%Y%m%d_%H%M%S}", now);
    return std::format("{}_{}.csv", base, suffix);
}
} // namespace ts::time
