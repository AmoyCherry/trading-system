#include "engine.hpp"                 // your engine.hpp
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

  const std::string local = arg(argc, argv, "--local", "/tmp/ts_lob.sock");
  const std::string to_gateway = arg(argc, argv, "--to-gateway", "/tmp/ts_gw.sock");

  ts::transport::UdsDgramSocket sock(local);
  const auto gw_peer = ts::transport::UdsDgramSocket::peer_from_path(to_gateway);

  ts::engine::Engine eng;

  std::cout << "READY lobd local=" << local << " to_gateway=" << to_gateway << "\n" << std::flush;

  while (!g_stop.load()) {
    ts::wire::Frame rx;
    ts::transport::Peer from{};
    const auto n = sock.recv_into(rx.writable(), from);
    rx.len = static_cast<std::uint32_t>(n);

    auto d = ts::wire::decode(rx.bytes_view());
    if (!d.has_value()) continue;

    const auto seq = d->seq;

    ts::engine::EventSink out = [&](const ts::engine::Event& ev) {
      std::visit([&](auto&& inner) {
        ts::wire::Frame tx;
        ts::wire::encode(inner, seq, tx);
        sock.send_to(tx.bytes_view(), gw_peer);
      }, ev);
    };

    // Only accept client messages
    if (auto* m = std::get_if<ts::proto::NewOrder>(&d->msg)) {
      eng.on_new(*m, out);
    } else if (auto* c = std::get_if<ts::proto::Cancel>(&d->msg)) {
      eng.on_cancel(*c, out);
    } else {
      continue;
    }

    // Mark end of response for this seq
    // Because matching may emit multiple events per imc
    {
      ts::wire::Frame endf;
      ts::wire::encode(ts::proto::ResponseEnd{}, seq, endf);
      sock.send_to(endf.bytes_view(), gw_peer);
    }
  }

  return 0;
}