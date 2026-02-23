#pragma once
#include <algorithm>
#include <vector>

namespace ts::stats {

struct SummaryNs {
    std::uint64_t p50{};
    std::uint64_t p90{};
    std::uint64_t p99{};
    std::uint64_t p999{};
    std::uint64_t max{};
};

inline std::uint64_t pick(std::vector<std::uint64_t>& v, double q) {
    if (v.empty()) return 0;
    const auto idx = static_cast<std::size_t>(q * (v.size() - 1));
    std::nth_element(v.begin(), v.begin() + idx, v.end());
    return v[idx];
}

inline SummaryNs summarize_ns(std::vector<std::uint64_t> v) {
    SummaryNs s;
    if (v.empty()) return s;
    s.p50  = pick(v, 0.50);
    s.p90  = pick(v, 0.90);
    s.p99  = pick(v, 0.99);
    s.p999 = pick(v, 0.999);
    s.max  = *std::max_element(v.begin(), v.end());
    return s;
}

} // namespace ts::stats
