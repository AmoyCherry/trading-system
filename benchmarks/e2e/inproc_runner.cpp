#include "inproc_gateway.hpp"
#include "protocol/pipeline.hpp"
#include "stats/percentiles.hpp"
#include "time/clock.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

using ts::proto::ClientMsg;
using ts::proto::NewOrder;
using ts::proto::Cancel;
using ts::proto::Side;

struct Counters {
  std::uint64_t acks = 0;
  std::uint64_t cancels = 0;
  std::uint64_t rejects = 0;
  std::uint64_t fills = 0;

  void on_event(const ts::engine::Event& ev) {
    if (std::holds_alternative<ts::proto::OrderAck>(ev)) ++acks;
    else if (std::holds_alternative<ts::proto::CancelAck>(ev)) ++cancels;
    else if (std::holds_alternative<ts::proto::Reject>(ev)) ++rejects;
    else if (std::holds_alternative<ts::proto::Fill>(ev)) ++fills;
  }
};

// ---- MANUAL ROI: scenario design ----
// Keep deterministic, simple, and interpretable.
std::vector<ClientMsg> make_scenario_cross(std::uint64_t pairs) {
  std::vector<ClientMsg> v;
  v.reserve(static_cast<std::size_t>(pairs) * 2);

  std::uint64_t id = 1;
  for (std::uint64_t i = 0; i < pairs; ++i) {
    // resting sell
    v.emplace_back(NewOrder{id++, Side::Sell, 10'000, 1, 0});
    // aggressive buy crosses immediately
    v.emplace_back(NewOrder{id++, Side::Buy,  20'000, 1, 0});
  }
  return v;
}

std::vector<ClientMsg> make_scenario_add_only(std::uint64_t n) {
  std::vector<ClientMsg> v;
  v.reserve(static_cast<std::size_t>(n));
  std::uint64_t id = 1;
  for (std::uint64_t i = 0; i < n; ++i) {
    const auto px = static_cast<ts::proto::Price>(10'000 + (i % 64));
    v.emplace_back(NewOrder{id++, Side::Buy, px, 1, 0});
  }
  return v;
}

} // namespace

int main(int argc, char** argv) {
  std::string scenario = "cross";
  std::uint64_t n = 1'000'000;
  std::uint64_t warmup = 10'000;

  // Minimal CLI
  if (argc >= 2) scenario = argv[1];
  if (argc >= 3) n = std::stoull(argv[2]);
  if (argc >= 4) warmup = std::stoull(argv[3]);

  ts::engine::Engine eng;
  ts::gw::InProcGateway gw(eng);

  std::vector<ClientMsg> stream;
  if (scenario == "cross") stream = make_scenario_cross(n);
  else if (scenario == "add") stream = make_scenario_add_only(n);
  else {
    std::cerr << "Unknown scenario. Use: cross | add\n";
    return 2;
  }

  Counters ctr;

  std::vector<std::uint64_t> total_ns;
  std::vector<std::uint64_t> lob_ns;
  total_ns.reserve(stream.size());
  lob_ns.reserve(stream.size());

  // Warmup
  for (std::uint64_t i = 0; i < warmup && i < stream.size(); ++i) {
    gw.handle(stream[i], [&](const ts::engine::Event& ev) { ctr.on_event(ev); });
  }

  const auto bench_start = ts::time::now_ns();
  for (std::size_t i = warmup; i < stream.size(); ++i) {
    const auto t0 = ts::time::now_ns();
    auto m = gw.handle(stream[i], [&](const ts::engine::Event& ev) { ctr.on_event(ev); });
    const auto t1 = ts::time::now_ns();

    // Definition: end-to-end in-proc latency for one request
    total_ns.push_back(t1 - t0);

    // Attribution: time measured around engine call (already inside gateway)
    lob_ns.push_back(m.lob_ns);
  }
  const auto bench_end = ts::time::now_ns();

  const auto total_sum = ts::stats::summarize_ns(total_ns);
  const auto lob_sum   = ts::stats::summarize_ns(lob_ns);

  const double seconds = (bench_end - bench_start) / 1e9;
  const double ops = static_cast<double>(total_ns.size());
  const double throughput = (seconds > 0.0) ? (ops / seconds) : 0.0;

  std::cout << "scenario=" << scenario << " n=" << stream.size()
            << " warmup=" << warmup << "\n";
  std::cout << "throughput_ops_per_s=" << throughput << "\n\n";

  std::cout << "[inproc total latency ns] p50=" << total_sum.p50
            << " p99=" << total_sum.p99
            << " p99.9=" << total_sum.p999
            << " max=" << total_sum.max << "\n";

  std::cout << "[engine-only approx ns]   p50=" << lob_sum.p50
            << " p99=" << lob_sum.p99
            << " p99.9=" << lob_sum.p999
            << " max=" << lob_sum.max << "\n\n";

  std::cout << "events: acks=" << ctr.acks
            << " fills=" << ctr.fills
            << " cancels=" << ctr.cancels
            << " rejects=" << ctr.rejects << "\n";

  return 0;
}
