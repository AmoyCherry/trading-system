#pragma once

#include "protocol/messages.hpp"

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <variant>

#include "../book_summary.hpp"

namespace ts::engine {

namespace proto = ts::proto;

using Event = std::variant<proto::OrderAck, proto::CancelAck, proto::Reject, proto::Fill>;
using EventSink = std::function<void(const Event&)>;

class OrderBook {
public:
    void on_new(const proto::NewOrder& msg, const EventSink& out);
    void on_cancel(const proto::Cancel& msg, const EventSink& out);

    // Debug/query helpers (high ROI for tests + later perf attribution)
    std::size_t live_order_count() const { return live_.size(); }
    bool has_order(proto::OrderId id) const { return live_.find(id) != live_.end(); }
    proto::Qty order_qty(proto::OrderId id) const;
    std::optional<proto::Price> best_bid() const;
    std::optional<proto::Price> best_ask() const;

    BookSummary summary() const;

    // observer
    struct LevelStates {
        proto::Price price{};
        proto::Qty qty{};
        std::size_t order_count{};
    };

    std::uint32_t active_levels(proto::Side side) const;
    std::vector<proto::OrderId> order_ids_at_price(proto::Side side, proto::Price price) const;
    std::vector<LevelStates> level_stats(proto::Side side, std::size_t limit) const;

private:
    struct LiveOrder {
        proto::OrderId id{};
        proto::Symbol symbol{};
        proto::Side side{};
        proto::Price price{};
        proto::Qty qty{};
    };

    using Level = std::deque<proto::OrderId>;

    // Best bid and best ask are begin() for both due to comparator choices.
    std::map<proto::Price, Level, std::greater<proto::Price>> bids_;
    std::map<proto::Price, Level> asks_;

    std::unordered_map<proto::OrderId, LiveOrder> live_;

    void match_buy(LiveOrder& incoming, const EventSink& out);
    void match_sell(LiveOrder& incoming, const EventSink& out);

    void add_resting(const LiveOrder& o);

    bool erase_from_bids(proto::Price price, proto::OrderId id);
    bool erase_from_asks(proto::Price price, proto::OrderId id);
};

} // namespace ts::engine
