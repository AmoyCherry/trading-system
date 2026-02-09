#pragma once

#include "book/order_book.hpp"

namespace ts::engine {

class Engine {
public:
    void on_new(const proto::NewOrder& msg, const EventSink& out) { book_.on_new(msg, out); }
    void on_cancel(const proto::Cancel& msg, const EventSink& out) { book_.on_cancel(msg, out); }

    // queries (used by tests; also useful later for sanity checks)
    std::size_t live_order_count() const { return book_.live_order_count(); }
    bool has_order(proto::OrderId id) const { return book_.has_order(id); }
    proto::Qty order_qty(proto::OrderId id) const { return book_.order_qty(id); }
    std::optional<proto::Price> best_bid() const { return book_.best_bid(); }
    std::optional<proto::Price> best_ask() const { return book_.best_ask(); }

private:
    OrderBook book_;
};

} // namespace ts::engine
