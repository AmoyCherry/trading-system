#pragma once

#include <cstdint>
#include <optional>

#include "protocol/messages.hpp"

namespace ts::engine {

struct BookSummary {
    std::uint64_t live_orders = 0;
    std::optional<ts::proto::Price> best_ask{};
    std::optional<ts::proto::Price> best_bid{};
    std::uint64_t state_hash = 0;
};

inline std::uint64_t price_or_na(std::optional<ts::proto::Price> p) {
    return p.has_value() ? static_cast<std::uint64_t>(*p) : -1;
}

}
