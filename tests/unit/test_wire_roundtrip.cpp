#include <gtest/gtest.h>
#include "protocol/wire.hpp"

using namespace ts;

TEST(WireCodec, RoundTripNewOrder) {
    proto::NewOrder in{123, proto::Side::Buy, 10000, 7, 42};
    wire::Frame f;
    ASSERT_TRUE(wire::encode(in, /*seq=*/999, f));

    auto d = wire::decode(f.bytes_view());
    ASSERT_TRUE(d.has_value());
    EXPECT_EQ(d->version, wire::kWireVersion);
    EXPECT_EQ(d->seq, 999);

    auto* out = std::get_if<proto::NewOrder>(&d->msg);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->id, in.id);
    EXPECT_EQ(static_cast<int>(out->side), static_cast<int>(in.side));
    EXPECT_EQ(out->price, in.price);
    EXPECT_EQ(out->qty, in.qty);
    EXPECT_EQ(out->symbol, in.symbol);
}

TEST(WireCodec, RoundTripFill) {
    proto::Fill in{1, 2, 10000, 5, true};
    wire::Frame f;
    ASSERT_TRUE(wire::encode(in, /*seq=*/7, f));

    auto d = wire::decode(f.bytes_view());
    ASSERT_TRUE(d.has_value());

    auto* out = std::get_if<proto::Fill>(&d->msg);
    ASSERT_NE(out, nullptr);
    EXPECT_EQ(out->id, in.id);
    EXPECT_EQ(out->contra_id, in.contra_id);
    EXPECT_EQ(out->price, in.price);
    EXPECT_EQ(out->qty, in.qty);
    EXPECT_EQ(out->is_taker, in.is_taker);
}

TEST(WireCodec, RejectsBadLength) {
    proto::Cancel in{123};
    wire::Frame f;
    ASSERT_TRUE(wire::encode(in, 1, f));

    // Corrupt length: drop a byte
    auto bytes = f.bytes_view();
    bytes = bytes.subspan(0, bytes.size() - 1);

    auto d = wire::decode(bytes);
    EXPECT_FALSE(d.has_value());
}