#include <string>
#include <cstdint>
#include <iostream>

#include "scenario/scenarios.hpp"
#include "transport/uds_dgram.hpp"
#include "time/clock.hpp"
#include "protocol/wire.hpp"

namespace {

std::string arg(int argc, char** argv, const std::string& name, const std::string& def) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (name == argv[i]) return argv[i + 1];
    }
    return def;
}

std::uint64_t arg_u64(int argc, char** argv, const std::string& name, std::uint64_t def) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (name == argv[i]) return std::stoull(argv[i + 1]);
    }
    return def;
}

} // namespace

int main(int argc, char** argv) {
    const std::string local = arg(argc, argv, "--local", "/tmp/ts_exch.sock");
    const std::string to_gateway = arg(argc, argv, "--to-gateway", "/tmp/ts_gw.sock");
    const std::string scenario_name = arg(argc, argv, "--scenario", "cross");
    const std::uint64_t n = arg_u64(argc, argv, "--n", 200'000);

    const auto kind = ts::scenario::parse_kind(scenario_name);
    const auto stream = ts::scenario::make(kind, n);

    ts::transport::UdsDgramSocket sock(local);
    const auto peer = ts::transport::UdsDgramSocket::peer_from_path(local);

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

        sock.send_to(frame.bytes_view(), peer);
        ++seq;
    }

    {
        ts::wire::Frame endf;
        ts::wire::encode(ts::proto::EndOfReplay{}, seq, endf);
        sock.send_to(endf.bytes_view(), peer);
    }

    // throughput
    const auto t1 = ts::time::now_ns();
    const auto total_msgs = static_cast<std::uint64_t>(stream.size() + 1);
    const double seconds = (t1 - t0)/1e9;
    const auto throughput = seconds > 0.0 ? (static_cast<double>(total_msgs) / seconds) : 0.0;

    std::cout << "RESULT role=exchange"
            << " scenario=" << ts::scenario::to_string(kind)
            << " scenario_units=" << n
            << " total_msgs=" << total_msgs
            << " send_elapsed_ns=" << (t1 - t0)
            << " send_throughput_msgs_per_s=" << throughput
            << "\n";
    return 0;
}