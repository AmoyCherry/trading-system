#include <string>
#include <cstdint>
#include <iostream>
#include <format>

#include "generator/scenarios.hpp"
#include "transport/uds_dgram.hpp"
#include "time/clock.hpp"
#include "protocol/wire.hpp"
#include "stats/sample_buffer.hpp"
#include "util/affinity.hpp"

namespace {

std::string arg(int argc, char** argv, const std::string& name, const std::string& def) {
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

struct EXTS {
    std::uint64_t t_seq = 0;
    std::uint64_t t_ex_before_send = 0;
    std::uint64_t t_ex_after_send = 0;
}; // EX timestamp
} // namespace

int main(int argc, char** argv) {
    const std::string local = arg(argc, argv, "--local", "/tmp/ts_exch.sock");
    const std::string to_gateway = arg(argc, argv, "--to-gateway", "/tmp/ts_gw.sock");
    const std::string scenario_name = arg(argc, argv, "--scenario", "cross");
    const std::string dump_dir = arg(argc, argv, "--dump", "artifacts/dump");
    const std::uint64_t n = arg_u64(argc, argv, "--n", 200'000);
    const std::int64_t cpu_core = arg_i64(argc, argv, "--cpu-core", -1);
    const std::int64_t stride = arg_i64(argc, argv, "--stride", -1);

    if (cpu_core >= 0) {
        ts::util::pin_thread_to_cpu(cpu_core);
    }

    const auto kind = ts::gen::parse_kind(scenario_name);
    const auto& stream = ts::gen::make(kind, n);

    ts::transport::UdsDgramSocket sock(local);
    const auto peer = ts::transport::UdsDgramSocket::peer_from_path(to_gateway);

    std::vector<EXTS> extses;
    ts::stats::pre_faulting(extses, stride, stream.size());

    std::uint64_t seq = 1;
    const auto t0 = ts::time::now_ns();

    for (const auto& msg : stream) {
        ts::wire::Frame frame{};
        const bool ok = std::visit([&](auto&& inner) {
            return ts::wire::encode(inner, seq, frame);
        }, msg);
        if (!ok) {
            std::cerr << "encode failed for seq=" << "\n";
            return 2;
        }

        uint64_t before_send = 0;
        const bool hit = ts::stats::should_sample(seq, stride);
        if (hit) {
            before_send = ts::time::now_ns();
        }

        sock.send_to(frame.bytes_view(), peer);

        if (hit) {
            const uint64_t after_send = ts::time::now_ns();
            extses.emplace_back(seq, before_send, after_send);
        }
        ++seq;
    }

    {
        // EOF not counted in csv
        ts::wire::Frame endf;
        ts::wire::encode(ts::proto::EndOfReplay{}, seq, endf);
        sock.send_to(endf.bytes_view(), peer);
    }

    // throughput
    const auto t1 = ts::time::now_ns();
    const auto total_msgs = static_cast<std::uint64_t>(stream.size() + 1);
    const double seconds = (t1 - t0)/1e9;
    const auto throughput = seconds > 0.0 ? (static_cast<double>(total_msgs) / seconds) : 0.0;

    const auto& filename = std::format("{}/{}", dump_dir, "exts.csv");
    ts::stats::dump(extses, "seq,ex_before_send,ex_after_send", filename,
        [](const EXTS& exts) {
            return std::format("{},{},{}", exts.t_seq, exts.t_ex_before_send, exts.t_ex_after_send);
        });

    std::cout << "RESULT role=exchange"
            << " scenario=" << ts::gen::to_string(kind)
            << " scenario_units=" << n
            << " total_msgs=" << total_msgs
            << " send_elapsed_ns=" << (t1 - t0)
            << " send_throughput_msgs_per_s=" << throughput
            << "\n";
    return 0;
}