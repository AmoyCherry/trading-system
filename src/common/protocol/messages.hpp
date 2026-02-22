#pragma once

#include <cstdint>

namespace ts::proto {

    using OrderId = std::uint64_t;
    using Symbol  = std::uint32_t;

    // v0: integer ticks (avoid floating point surprises)
    using Price = std::int32_t;
    using Qty   = std::int32_t;

    enum class Side : std::uint8_t { Buy = 0, Sell = 1 };

    struct NewOrder {
        OrderId id{};
        Side side{};
        Price price{};
        Qty qty{};
        Symbol symbol{0};
    };

    struct Cancel {
        OrderId id{};
    };

    struct OrderAck {
        OrderId id{};
    };

    struct CancelAck {
        OrderId id{};
    };

    enum class RejectReason : std::uint8_t {
        DuplicateOrderId = 0,
        UnknownOrderId   = 1,
        BadQty           = 2,
        BadPrice         = 3,
      };

    struct Reject {
        OrderId id{};
        RejectReason reason{};
    };

    // v0 Fill: one event per order (so you emit 2 per trade)
    struct Fill {
        OrderId id{};          // the order being filled
        OrderId contra_id{};   // the counterparty order id
        Price price{};
        Qty qty{};
        bool is_taker{false};
    };

} // namespace ts::proto
