#include "engine.hpp"                 // your engine.hpp
#include "protocol/wire.hpp"
#include "transport/uds_dgram.hpp"

#include <atomic>
#include <csignal>
#include <format>
#include <iostream>
#include <string>

#include "book_summary.hpp"
#include "stats/sample_buffer.hpp"
#include "time/clock.hpp"
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

static std::int64_t arg_i64(int argc, char** argv, const std::string& name, const std::int64_t def) {
  for (int i = 1; i + 1 < argc; ++i) {
    if (name == argv[i]) return std::stoll(argv[i + 1]);
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

struct LobTS {
  std::uint64_t t_seq = 0;
  std::uint64_t t_lob_recv = 0;
  std::uint64_t t_lob_decode_done = 0;
  std::uint64_t t_lob_apply_done = 0;
};

enum class Mode {
  Null,
  Decode,
  Match,
};

// template propagation
template <Mode M>
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
            << " best_ask=" << ts::engine::price_or_na(s.best_ask);

  if constexpr (M == Mode::Match) {
    std::cout << " state_hash=" << s.state_hash;
  }
  std::cout << "\n";
}

Mode parse_mode(std::string_view mode) {
  if (mode == "null") return Mode::Null;
  if (mode == "decode") return Mode::Decode;
  if (mode == "match") return Mode::Match;
  throw std::invalid_argument("invalid mode: " + std::string(mode));
}

template <Mode M>
int lobd_main(int argc, char** argv) {
  std::signal(SIGINT, on_sig);
  std::signal(SIGTERM, on_sig);

  const std::string local = arg(argc, argv, "--local", "/tmp/ts_lob.sock");
  const std::string dump_dir = arg(argc, argv, "--dump", "artifacts/dump");
  const std::int64_t cpu_core = arg_i64(argc, argv, "--cpu-core", -1);
  const std::int64_t stride = arg_i64(argc, argv, "--stride", -1);
  const std::int64_t total_msgs = arg_i64(argc, argv, "--msgs", -1);

  if (total_msgs < 0) { throw std::invalid_argument("total_msg must be greater than 0"); }

  if (cpu_core >= 0) {
    ts::util::pin_thread_to_cpu(cpu_core);
  }

  std::vector<LobTS> tses;
  ts::stats::pre_faulting(tses, stride, total_msgs);

  // It's a local counter different from upstream's seq
  std::uint64_t cnt = 1;

  ts::transport::UdsDgramSocket sock(local);

  ts::engine::Engine eng;
  ReplayCounters ctr;

  ts::engine::EventSink out = [&](const ts::engine::Event& ev) {
    ctr.on_event(ev);
  };

  std::cout << "READY lobd local=" << local << "\n" << std::flush;

  while (!g_stop.load()) {
    ts::wire::Frame frame;
    ts::transport::Peer from{};
    std::uint64_t t_recv = 0;
    const auto n = sock.recv_into(frame.writable(), from);
    const bool hit = ts::stats::should_sample(cnt++, stride);
    if (hit) {
      t_recv = ts::time::now_ns();
    }
    frame.len = static_cast<std::uint32_t>(n);

    auto m_type = ts::wire::decode_type(frame.bytes_view());
    if (m_type && *m_type == ts::wire::MsgType::EndOfReplay) {
      break;
    }

    if constexpr (M == Mode::Null) {  }
    else if constexpr (M == Mode::Decode || M == Mode::Match) {
      std::uint64_t t_decode_done = 0;
      auto d = ts::wire::decode(frame.bytes_view());
      if (hit) {
        t_decode_done = ts::time::now_ns();
      }
      if (!d.has_value()) {
        ++ctr.decode_errors;
        continue;
      }

      std::uint64_t t_apply_done = 0;
      if constexpr (M == Mode::Match) {
        // Only accept client messages
        if (auto* m = std::get_if<ts::proto::NewOrder>(&d->msg)) {
          ++ctr.applied_msgs;
          eng.on_new(*m, out);
        } else if (auto* c = std::get_if<ts::proto::Cancel>(&d->msg)) {
          ++ctr.applied_msgs;
          eng.on_cancel(*c, out);
        } else {
          // unexpected msg type
          std::cerr << "Unexpected msg type can't be handled in lob" << "\n";
          ++ctr.decode_errors;
        }

        if (hit) {
          t_apply_done = ts::time::now_ns();
        }
      } // if M Match
      // null mode only has t_recv, no any use to record the abs
      if (hit) {
        tses.emplace_back(d->seq, t_recv, t_decode_done, t_apply_done);
      }
    } // if Decode || Match
  } // while g_stop

  const auto& filename = std::format("{}/{}", dump_dir, "lobts.csv");
  ts::stats::dump(tses, "seq,lob_recv,lob_decode_done,lob_apply_done", filename,
    [](const LobTS& timestamp){
      return std::format("{},{},{},{}", timestamp.t_seq, timestamp.t_lob_recv, timestamp.t_lob_decode_done, timestamp.t_lob_apply_done);
    });

  print_result<M>(ctr, eng.summary());
  return 0;
}

}

int main(int argc, char** argv) {
  const Mode mode = parse_mode(arg(argc, argv, "--mode", ""));
  switch (mode) {
    case Mode::Null: return lobd_main<Mode::Null>(argc, argv);
    case Mode::Decode: return lobd_main<Mode::Decode>(argc, argv);
    case Mode::Match: return lobd_main<Mode::Match>(argc, argv);
  }
  return 0;
}