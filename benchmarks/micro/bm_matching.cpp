#include <benchmark/benchmark.h>

#include <cstdint>
#include <vector>

#include "book/order_book.hpp"
// If you benchmark Engine wrapper instead, include engine.hpp.
// #include "engine/engine.hpp"

namespace {
using ts::proto::NewOrder;
using ts::proto::Cancel;
using ts::proto::Side;
using ts::proto::Price;
using ts::proto::Qty;
using ts::proto::OrderId;

// Minimal sink: count events, avoid allocations, avoid printing.
struct CountingSink {
  std::uint64_t acks = 0;
  std::uint64_t cancels = 0;
  std::uint64_t rejects = 0;
  std::uint64_t fills = 0;

  void reset() { acks = cancels = rejects = fills = 0; }

  void operator()(const ts::engine::Event& ev) {
    if (std::holds_alternative<ts::proto::OrderAck>(ev)) ++acks;
    else if (std::holds_alternative<ts::proto::CancelAck>(ev)) ++cancels;
    else if (std::holds_alternative<ts::proto::Reject>(ev)) ++rejects;
    else if (std::holds_alternative<ts::proto::Fill>(ev)) ++fills;
  }
};

std::vector<NewOrder> make_resting_orders(
    Side side, Price start_price, int levels, int per_level, Qty qty, OrderId& next_id) {
  std::vector<NewOrder> v;
  v.reserve(static_cast<std::size_t>(levels) * static_cast<std::size_t>(per_level));
  for (int l = 0; l < levels; ++l) {
    Price px = (side == Side::Sell) ? (start_price + l) : (start_price - l);
    for (int k = 0; k < per_level; ++k) {
      v.push_back(NewOrder{next_id++, side, px, qty, 0});
    }
  }
  return v;
}

std::vector<Cancel> make_cancel_stream(const std::vector<NewOrder>& orders) {
  std::vector<Cancel> v;
  v.reserve(orders.size());
  for (const auto& o : orders) v.push_back(Cancel{o.id});
  return v;
}

} // namespace

// Benchmark 1: Add N resting orders into an empty book.
static void BM_AddOrders(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));

  // Deterministic order stream
  OrderId next_id = 1;
  std::vector<NewOrder> orders;
  orders.reserve(n);
  for (int i = 0; i < n; ++i) {
    // spread a bit across prices to avoid degenerate single-level behavior
    const Price px = 10'000 + (i % 32);
    orders.push_back(NewOrder{next_id++, Side::Buy, px, 1, 0});
  }

  ts::engine::OrderBook book;
  CountingSink sink;
  ts::engine::EventSink out = [&](const ts::engine::Event& e) { sink(e); };

  // This loop runs so many times for the current test case to get a stable avg measurement
  // The benchmark measures the perf within this loop, typically needs to pause for setup
  for (auto _ : state) {
    state.PauseTiming();
    book = ts::engine::OrderBook{};
    sink.reset();
    state.ResumeTiming();

    for (const auto& o : orders) book.on_new(o, out);

    benchmark::DoNotOptimize(sink.acks);
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::uint64_t>(n));
}
BENCHMARK(BM_AddOrders)->Arg(1'000)->Arg(10'000)->Arg(50'000);

// Benchmark 2: Cancel N existing orders.
static void BM_CancelOrders(benchmark::State& state) {
  const int n = static_cast<int>(state.range(0));

  OrderId next_id = 1;
  std::vector<NewOrder> orders;
  orders.reserve(n);
  for (int i = 0; i < n; ++i) {
    const Price px = 10'000 + (i % 32);
    orders.push_back(NewOrder{next_id++, Side::Buy, px, 1, 0});
  }
  const auto cancels = make_cancel_stream(orders);

  ts::engine::OrderBook book;
  CountingSink sink;
  ts::engine::EventSink out = [&](const ts::engine::Event& e) { sink(e); };

  for (auto _ : state) {
    state.PauseTiming();
    book = ts::engine::OrderBook{};
    sink.reset();
    for (const auto& o : orders) book.on_new(o, out);
    sink.reset(); // don't count setup acks
    state.ResumeTiming();

    for (const auto& c : cancels) book.on_cancel(c, out);

    benchmark::DoNotOptimize(sink.cancels);
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * static_cast<std::uint64_t>(n));
}
BENCHMARK(BM_CancelOrders)->Arg(1'000)->Arg(10'000)->Arg(100'000);

// Benchmark 3: Matching cost — one aggressive order sweeping multiple levels.
static void BM_MatchSweep(benchmark::State& state) {
  const int levels = static_cast<int>(state.range(0));
  const int per_level = static_cast<int>(state.range(1));
  const int total_resting = levels * per_level;

  OrderId next_id = 1;
  // Resting asks from 10000..10000+levels-1
  const auto resting = make_resting_orders(Side::Sell, 10'000, levels, per_level, /*qty=*/1, next_id);

  // Aggressive buy priced above best ask; size sweeps all resting.
  const NewOrder aggressive{next_id++, Side::Buy, 12'000, static_cast<Qty>(total_resting), 0};

  ts::engine::OrderBook book;
  CountingSink sink;
  ts::engine::EventSink out = [&](const ts::engine::Event& e) { sink(e); };

  for (auto _ : state) {
    state.PauseTiming();
    book = ts::engine::OrderBook{};
    sink.reset();
    for (const auto& o : resting) book.on_new(o, out);
    sink.reset(); // don't count setup acks
    state.ResumeTiming();

    book.on_new(aggressive, out);

    benchmark::DoNotOptimize(sink.fills);
    benchmark::ClobberMemory();
  }

  // Items processed = #resting matched (approx proxy for “work done”)
  state.SetItemsProcessed(state.iterations() * static_cast<std::uint64_t>(total_resting));
}
// Args: levels, per_level
BENCHMARK(BM_MatchSweep)->Args({16, 1024})->Args({64, 2049})->Args({128, 4099})->Args({3000, 628}/*partial sweep*/);

BENCHMARK_MAIN();
