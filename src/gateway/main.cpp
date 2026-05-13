#include "protocol/wire.hpp"
#include "transport/uds_dgram.hpp"
#include "util/affinity.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <string>

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

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sig);
    std::signal(SIGTERM, on_sig);

    const std::string local = arg(argc, argv, "--local", "/tmp/ts_gw.sock");
    const std::string to_lob = arg(argc, argv, "--to-lob", "/tmp/ts_lob.sock");
    const std::uint64_t cpu_core = arg_64(argc, argv, "--cpu-core", 3);

    // taskset -c MUST be disabled!
    ts::util::pin_thread_to_cpu(cpu_core);

    ts::transport::UdsDgramSocket sock(local);
    const auto lob_peer  = ts::transport::UdsDgramSocket::peer_from_path(to_lob);

    std::cout << "READY role=gateway local=" << local << " to_lob=" << to_lob << "\n" << std::flush;

    std::uint64_t decode_errors = 0;
    std::uint64_t forwarded = 0;
    std::uint64_t seq_errors = 0;
    std::uint64_t last_seq = 0;

    while (!g_stop.load()) {
        ts::wire::Frame frame{};
        ts::transport::Peer from{};
        const auto n = sock.recv_into(frame.writable(), from);
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

        sock.send_to(frame.bytes_view(), lob_peer);
        ++ forwarded;

        if (std::holds_alternative<ts::proto::EndOfReplay>(decoded->msg)) {
            break;
        }
    }

    std::cout << "RESULT role=gateway"
            << " forwarded=" << forwarded
            << " decode_errors=" << decode_errors
            << " seq_errors=" << seq_errors
            << " last_seq=" << last_seq
            << "\n";

    return 0;
}