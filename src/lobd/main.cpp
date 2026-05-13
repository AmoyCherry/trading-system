#include "engine.hpp"                 // your engine.hpp
#include "protocol/wire.hpp"
#include "transport/uds_dgram.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

#include "book_summary.hpp"
#include "util/affinity.hpp"

namespace {

static std::atomic<bool> g_stop{false};
static void on_sig(int) { g_stop.store(true); }

static std::string arg(int argc, char** argv, const std::string& name, const std::string& def) {
  for (int i = 1; i + 1 < argc; ++i) {
    if (name == argv[i]) return argv[i + 1];
  }
  return def;
}

static std::uint64_t arg_64(int argc, char** argv, const std::string& name, const std::uint64_t def) {
  for (int i = 1; i + 1 < argc; ++i) {
    if (name == argv[i]) return std::stoull(argv[i + 1]);
  }
  return def;
}

// acks + rejects + cancel_acks should equal to total_msgs, see order_book on_new and on_cancel
struct ReplayCounters {
  std::uint64_t acks = 0;
  std::uint64_t cancel_acks = 0;
  std::uint64_t fills = 0;
  std::uint64_t rejects = 0;

  std::uint64_t applied_msgs = 0;
  std::uint64_t decode_errors = 0;
  std::uint64_t seq_errors = 0;

  void on_event(const ts::engine::Event& ev) {
    if (std::holds_alternative<ts::proto::OrderAck>(ev)) ++acks;
    else if (std::holds_alternative<ts::proto::CancelAck>(ev)) ++cancel_acks;
    else if (std::holds_alternative<ts::proto::Fill>(ev)) ++fills;
    else if (std::holds_alternative<ts::proto::Reject>(ev)) ++rejects;
  }
};

void print_result(const ReplayCounters& ctr, const ts::engine::BookSummary& s) {
  std::cout << "STATS role=lobd"
           << " applied_msgs=" << ctr.applied_msgs
           << " decode_errors=" << ctr.decode_errors
           << " seq_errors=" << ctr.seq_errors
           << "\n";

  std::cout << "RESULT"
            << " acks=" << ctr.acks
            << " cancel_acks=" << ctr.cancel_acks
            << " rejects=" << ctr.rejects
            << " fills=" << ctr.fills
            << " live_orders=" << s.live_orders
            << " best_bid=" << ts::engine::price_or_na(s.best_bid)
            << " best_ask=" << ts::engine::price_or_na(s.best_ask)
            << " state_hash=" << s.state_hash
            << "\n";
}

}

int main(int argc, char** argv) {
  std::signal(SIGINT, on_sig);
  std::signal(SIGTERM, on_sig);

  const std::string local = arg(argc, argv, "--local", "/tmp/ts_lob.sock");
  const std::uint64_t cpu_core = arg_64(argc, argv, "--cpu-core", 4);
  // taskset -c MUST be disabled!
  ts::util::pin_thread_to_cpu(cpu_core);
  ts::transport::UdsDgramSocket sock(local);

  ts::engine::Engine eng;
  ReplayCounters ctr;

  std::cout << "READY lobd local=" << local << "\n" << std::flush;

  while (!g_stop.load()) {
    ts::wire::Frame frame;
    ts::transport::Peer from{};
    const auto n = sock.recv_into(frame.writable(), from);
    frame.len = static_cast<std::uint32_t>(n);

    auto d = ts::wire::decode(frame.bytes_view());
    if (!d.has_value()) {
      ++ctr.decode_errors;
      continue;
    }

    ts::engine::EventSink out = [&](const ts::engine::Event& ev) {
      ctr.on_event(ev);
    };

    // Only accept client messages
    if (auto* m = std::get_if<ts::proto::NewOrder>(&d->msg)) {
      ++ctr.applied_msgs;
      eng.on_new(*m, out);
    } else if (auto* c = std::get_if<ts::proto::Cancel>(&d->msg)) {
      eng.on_cancel(*c, out);
    } else if (auto* e = std::get_if<ts::proto::EndOfReplay>(&d->msg)) {
      break;
    } else {
      // unexpected msg type
      std::cerr << "Unexpected msg type can't be handled in lob" << "\n";
      ++ctr.decode_errors;
    }
  }

  print_result(ctr, eng.summary());
  return 0;
}