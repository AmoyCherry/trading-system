#include "protocol/wire.hpp"
#include "transport/uds_dgram.hpp"

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

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sig);
    std::signal(SIGTERM, on_sig);

    const std::string local = arg(argc, argv, "--local", "/tmp/ts_gw.sock");
    const std::string to_exchange = arg(argc, argv, "--to-exchange", "/tmp/ts_exch.sock");
    const std::string to_lob = arg(argc, argv, "--to-lob", "/tmp/ts_lob.sock");

    ts::transport::UdsDgramSocket sock(local);
    const auto exch_peer = ts::transport::UdsDgramSocket::peer_from_path(to_exchange);
    const auto lob_peer  = ts::transport::UdsDgramSocket::peer_from_path(to_lob);

    std::cout << "READY gateway local=" << local << " to_exchange=" << to_exchange
              << " to_lob=" << to_lob << "\n" << std::flush;

    enum class State { WaitClient, WaitLob };
    State st = State::WaitClient;
    std::uint64_t inflight_seq = 0;

    while (!g_stop.load()) {
        ts::wire::Frame rx;
        ts::transport::Peer from{};
        const auto n = sock.recv_into(rx.writable(), from);
        rx.len = static_cast<std::uint32_t>(n);

        auto d = ts::wire::decode(rx.bytes_view());
        if (!d.has_value()) continue;

        if (st == State::WaitClient) {
            // Forward client request to lob
            inflight_seq = d->seq;
            sock.send_to(rx.bytes_view(), lob_peer);
            st = State::WaitLob;
            continue;
        }

        // WaitLob: forward all responses to exchange until ResponseEnd(seq)
        sock.send_to(rx.bytes_view(), exch_peer);

        const bool is_end = std::holds_alternative<ts::proto::ResponseEnd>(d->msg);
        if (is_end && d->seq == inflight_seq) {
            st = State::WaitClient;
        }
    }

    return 0;
}