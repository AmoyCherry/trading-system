#include "book/order_book.hpp"

#include <algorithm>

namespace ts::engine {

proto::Qty OrderBook::order_qty(proto::OrderId id) const {
  auto it = live_.find(id);
  if (it == live_.end()) return 0;
  return it->second.qty;
}

std::optional<proto::Price> OrderBook::best_bid() const {
  if (bids_.empty()) return std::nullopt;
  return bids_.begin()->first;
}

std::optional<proto::Price> OrderBook::best_ask() const {
  if (asks_.empty()) return std::nullopt;
  return asks_.begin()->first;
}

void OrderBook::on_new(const proto::NewOrder& msg, const EventSink& out) {
  if (msg.qty <= 0) {
    out(proto::Reject{msg.id, proto::RejectReason::BadQty});
    return;
  }
  if (msg.price <= 0) {
    out(proto::Reject{msg.id, proto::RejectReason::BadPrice});
    return;
  }
  if (live_.find(msg.id) != live_.end()) {
    out(proto::Reject{msg.id, proto::RejectReason::DuplicateOrderId});
    return;
  }

  // Accepted
  out(proto::OrderAck{msg.id});

  LiveOrder incoming{msg.id, msg.symbol, msg.side, msg.price, msg.qty};

  if (incoming.side == proto::Side::Buy) {
    match_buy(incoming, out);
  } else {
    match_sell(incoming, out);
  }

  if (incoming.qty > 0) {
    add_resting(incoming);
  }
}

void OrderBook::on_cancel(const proto::Cancel& msg, const EventSink& out) {
  auto it = live_.find(msg.id);
  if (it == live_.end()) {
    out(proto::Reject{msg.id, proto::RejectReason::UnknownOrderId});
    return;
  }

  const auto side = it->second.side;
  const auto price = it->second.price;

  bool removed = false;
  if (side == proto::Side::Buy) removed = erase_from_bids(price, msg.id);
  else removed = erase_from_asks(price, msg.id);

  // Even if something went inconsistent, we treat it as removed from live_
  // because live_ is the source of truth for "exists".
  live_.erase(it);

  if (removed) {
    out(proto::CancelAck{msg.id});
  } else {
    // If we couldn't find it in the level container, treat as unknown/inconsistent.
    out(proto::Reject{msg.id, proto::RejectReason::UnknownOrderId});
  }
}

void OrderBook::add_resting(const LiveOrder& o) {
  live_.emplace(o.id, o);
  if (o.side == proto::Side::Buy) {
    bids_[o.price].push_back(o.id);
  } else {
    asks_[o.price].push_back(o.id);
  }
}

bool OrderBook::erase_from_bids(proto::Price price, proto::OrderId id) {
  auto it = bids_.find(price);
  if (it == bids_.end()) return false;
  auto& level = it->second;

  auto pos = std::find(level.begin(), level.end(), id);
  if (pos == level.end()) return false;

  level.erase(pos);
  if (level.empty()) bids_.erase(it);
  return true;
}

bool OrderBook::erase_from_asks(proto::Price price, proto::OrderId id) {
  auto it = asks_.find(price);
  if (it == asks_.end()) return false;
  auto& level = it->second;

  auto pos = std::find(level.begin(), level.end(), id);
  if (pos == level.end()) return false;

  level.erase(pos);
  if (level.empty()) asks_.erase(it);
  return true;
}

void OrderBook::match_buy(LiveOrder& incoming, const EventSink& out) {
  while (incoming.qty > 0 && !asks_.empty()) {
    auto best_ask_it = asks_.begin();
    const proto::Price px = best_ask_it->first;
    if (px > incoming.price) break;

    auto& level = best_ask_it->second;
    while (incoming.qty > 0 && !level.empty()) {
      const proto::OrderId maker_id = level.front();
      auto maker_it = live_.find(maker_id);
      if (maker_it == live_.end()) {
        // Inconsistent state; drop it.
        level.pop_front();
        continue;
      }

      auto& maker = maker_it->second;
      const proto::Qty traded = std::min(incoming.qty, maker.qty);

      // Emit fills: taker first, maker second (deterministic ordering)
      out(proto::Fill{incoming.id, maker_id, px, traded, true});
      out(proto::Fill{maker_id, incoming.id, px, traded, false});

      incoming.qty -= traded;
      maker.qty -= traded;

      if (maker.qty == 0) {
        level.pop_front();
        live_.erase(maker_it);
      }
    }

    if (level.empty()) asks_.erase(best_ask_it);
  }
}

void OrderBook::match_sell(LiveOrder& incoming, const EventSink& out) {
  while (incoming.qty > 0 && !bids_.empty()) {
    auto best_bid_it = bids_.begin();
    const proto::Price px = best_bid_it->first;
    if (px < incoming.price) break;

    auto& level = best_bid_it->second;
    while (incoming.qty > 0 && !level.empty()) {
      const proto::OrderId maker_id = level.front();
      auto maker_it = live_.find(maker_id);
      if (maker_it == live_.end()) {
        level.pop_front();
        continue;
      }

      auto& maker = maker_it->second;
      const proto::Qty traded = std::min(incoming.qty, maker.qty);

      out(proto::Fill{incoming.id, maker_id, px, traded, true});
      out(proto::Fill{maker_id, incoming.id, px, traded, false});

      incoming.qty -= traded;
      maker.qty -= traded;

      if (maker.qty == 0) {
        level.pop_front();
        live_.erase(maker_it);
      }
    }

    if (level.empty()) bids_.erase(best_bid_it);
  }
}

} // namespace ts::engine
