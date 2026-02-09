#include <gtest/gtest.h>

#include "engine.hpp"
#include "protocol/messages.hpp"
#include <vector>

namespace {

struct Collector {
  std::vector<ts::engine::Event> events;
  void operator()(const ts::engine::Event& e) { events.push_back(e); }
};

} // namespace

TEST(EngineBasic, SimpleCrossProducesFillsAndLeavesRemainder) {
  ts::engine::Engine eng;
  Collector out;

  // Resting sell: 10 @ 100
  eng.on_new(ts::proto::NewOrder{1, ts::proto::Side::Sell, 100, 10, 0}, out);
  // Aggressive buy: 7 @ 100
  eng.on_new(ts::proto::NewOrder{2, ts::proto::Side::Buy,  100,  7, 0}, out);

  // Events:
  // Ack(1), Ack(2), Fill(taker=2), Fill(maker=1)
  ASSERT_GE(out.events.size(), 4u);

  EXPECT_TRUE(std::holds_alternative<ts::proto::OrderAck>(out.events[0]));
  EXPECT_TRUE(std::holds_alternative<ts::proto::OrderAck>(out.events[1]));
  EXPECT_TRUE(std::holds_alternative<ts::proto::Fill>(out.events[2]));
  EXPECT_TRUE(std::holds_alternative<ts::proto::Fill>(out.events[3]));

  const auto f_taker = std::get<ts::proto::Fill>(out.events[2]);
  const auto f_maker = std::get<ts::proto::Fill>(out.events[3]);

  EXPECT_EQ(f_taker.id, 2u);
  EXPECT_EQ(f_taker.contra_id, 1u);
  EXPECT_EQ(f_taker.price, 100);
  EXPECT_EQ(f_taker.qty, 7);
  EXPECT_TRUE(f_taker.is_taker);

  EXPECT_EQ(f_maker.id, 1u);
  EXPECT_EQ(f_maker.contra_id, 2u);
  EXPECT_EQ(f_maker.price, 100);
  EXPECT_EQ(f_maker.qty, 7);
  EXPECT_FALSE(f_maker.is_taker);

  // Maker should have 3 remaining; taker should be gone.
  EXPECT_TRUE(eng.has_order(1));
  EXPECT_EQ(eng.order_qty(1), 3);
  EXPECT_FALSE(eng.has_order(2));

  // Book should not be crossed.
  const auto bb = eng.best_bid();
  const auto ba = eng.best_ask();
  if (bb && ba) EXPECT_LT(*bb, *ba);
}

TEST(EngineBasic, FIFOAtSamePrice) {
  ts::engine::Engine eng;
  Collector out;

  eng.on_new(ts::proto::NewOrder{1, ts::proto::Side::Sell, 100, 5, 0}, out);
  eng.on_new(ts::proto::NewOrder{2, ts::proto::Side::Sell, 100, 5, 0}, out);
  eng.on_new(ts::proto::NewOrder{3, ts::proto::Side::Buy,  100, 6, 0}, out);

  // Find taker fills for order 3 in order; should hit maker 1 then maker 2.
  std::vector<ts::proto::OrderId> contra;
  for (const auto& ev : out.events) {
    if (auto p = std::get_if<ts::proto::Fill>(&ev)) {
      if (p->id == 3 && p->is_taker) contra.push_back(p->contra_id);
    }
  }
  ASSERT_EQ(contra.size(), 2u);
  EXPECT_EQ(contra[0], 1u);
  EXPECT_EQ(contra[1], 2u);

  EXPECT_FALSE(eng.has_order(1));
  EXPECT_TRUE(eng.has_order(2));
  EXPECT_EQ(eng.order_qty(2), 4);
}

TEST(EngineBasic, CancelRemovesOrder) {
  ts::engine::Engine eng;
  Collector out;

  eng.on_new(ts::proto::NewOrder{10, ts::proto::Side::Buy, 99, 1, 0}, out);
  EXPECT_TRUE(eng.has_order(10));

  eng.on_cancel(ts::proto::Cancel{10}, out);
  EXPECT_FALSE(eng.has_order(10));

  // Cancel again -> reject
  eng.on_cancel(ts::proto::Cancel{10}, out);
  bool saw_unknown = false;
  for (const auto& ev : out.events) {
    if (auto r = std::get_if<ts::proto::Reject>(&ev)) {
      if (r->id == 10 && r->reason == ts::proto::RejectReason::UnknownOrderId) {
        saw_unknown = true;
      }
    }
  }
  EXPECT_TRUE(saw_unknown);
}
