#include "protocol/wire.hpp"
#include "transport/uds_dgram.hpp"
#include "util/affinity.hpp"
#include "stats/sample_buffer.hpp"
#include "time/clock.hpp"

#include <atomic>
#include <csignal>
#include <format>
#include <iostream>
#include <string>
#include <vector>

static std::atomic<bool> g_stop{false};
static void on_sig(int) { g_stop.store(true); }

static std::string arg(int argc, char** argv, const std::string& name, const std::string& def) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (name == argv[i]) return argv[i + 1];
    }
    return def;
}

static std::uint64_t arg_u64(int argc, char** argv, const std::string& name, const std::uint64_t def) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (name == argv[i]) return std::stoull(argv[i + 1]);
    }
    return def;
}

static std::int64_t arg_i64(int argc, char** argv, const std::string& name, const std::int64_t def) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (name == argv[i]) return std::stoll(argv[i + 1]);
    }
    return def;
}

struct GWTS {
    std::uint64_t t_seq = 0;
    std::uint64_t t_gw_recv = 0;
    std::uint64_t t_gw_before_send = 0;
    std::uint64_t t_gw_after_send = 0;
};

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sig);
    std::signal(SIGTERM, on_sig);

    const std::string local = arg(argc, argv, "--local", "/tmp/ts_gw.sock");
    const std::string to_lob = arg(argc, argv, "--to-lob", "/tmp/ts_lob.sock");
    const std::string dump_dir = arg(argc, argv, "--dump", "artifacts/dump");
    const std::int64_t cpu_core = arg_i64(argc, argv, "--cpu-core", -1);
    const std::int64_t stride = arg_i64(argc, argv, "--stride", -1);
    const std::int64_t total_msg = arg_i64(argc, argv, "--msgs", -1);

    if (total_msg < 0) { throw std::invalid_argument("total_msg must be greater than 0"); }

    if (cpu_core >= 0) {
        ts::util::pin_thread_to_cpu(cpu_core);
    }

    ts::transport::UdsDgramSocket sock(local);
    const auto lob_peer  = ts::transport::UdsDgramSocket::peer_from_path(to_lob);

    std::cout << "READY role=gateway local=" << local << " to_lob=" << to_lob << "\n" << std::flush;

    std::vector<GWTS> gwtses;
    ts::stats::pre_faulting(gwtses, stride, total_msg);
    std::uint64_t cnt = 1;

    std::uint64_t decode_errors = 0;
    std::uint64_t forwarded = 0;
    std::uint64_t seq_errors = 0;
    std::uint64_t last_seq = 0;

    while (!g_stop.load()) {
        ts::wire::Frame frame{};
        ts::transport::Peer from{};
        std::uint64_t t_recv = 0;
        const auto n = sock.recv_into(frame.writable(), from);
        const bool hit = ts::stats::should_sample(cnt++, stride);
        if (hit) {
            t_recv = ts::time::now_ns();
        }
        frame.len = static_cast<uint32_t>(n);

        const auto decoded = ts::wire::decode(frame.bytes_view());
        if (!decoded.has_value()) {
            ++decode_errors;
            continue;
        }

        if (decoded->seq != last_seq + 1) {
            ++seq_errors;
        }
        last_seq = decoded->seq;

        std::uint64_t t_before_send = 0;
        if (hit) {
            t_before_send = ts::time::now_ns();
        }

        sock.send_to(frame.bytes_view(), lob_peer);

        std::uint64_t t_after_send = 0;
        if (hit) {
            t_after_send = ts::time::now_ns();
            gwtses.emplace_back(decoded->seq, t_recv, t_before_send, t_after_send);
        }
        ++ forwarded;

        if (std::holds_alternative<ts::proto::EndOfReplay>(decoded->msg)) {
            break;
        }
    }

    const auto& filename = std::format("{}/{}", dump_dir, "gwts.csv");
    ts::stats::dump(gwtses,"seq,gw_recv,gw_before_send,gw_after_send",filename,
        [](const GWTS& timestamp) {
            return std::format("{},{},{},{}", timestamp.t_seq, timestamp.t_gw_recv, timestamp.t_gw_before_send, timestamp.t_gw_after_send);
        });

    std::cout << "RESULT role=gateway"
            << " forwarded=" << forwarded
            << " decode_errors=" << decode_errors
            << " seq_errors=" << seq_errors
            << " last_seq=" << last_seq
            << "\n";

    return 0;
}