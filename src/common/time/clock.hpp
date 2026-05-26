#pragma once
#include <cstdint>
#include <string>

namespace ts::time {
std::uint64_t now_ns() noexcept;
std::string get_filename_with_ymds(const std::string& base);
} // namespace ts::time
