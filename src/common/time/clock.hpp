#pragma once
#include <cstdint>
#include <string>

namespace ts::time {
std::uint64_t now_ns() noexcept;
constexpr std::uint64_t T = 4'000; // ns
} // namespace ts::time