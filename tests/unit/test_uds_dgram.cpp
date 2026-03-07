#include <gtest/gtest.h>
#include "transport/uds_dgram.hpp"

#include <unistd.h>
#include <string>
#include <vector>

using namespace ts::transport;

static std::string tmp_path(const char* name) {
    return std::string("/tmp/") + name + "_" + std::to_string(::getpid()) + ".sock";
}

TEST(UdsDgram, SendReceive) {
    const auto a_path = tmp_path("ts_a");
    const auto b_path = tmp_path("ts_b");

    UdsDgramSocket a(a_path);
    UdsDgramSocket b(b_path);

    auto b_peer = UdsDgramSocket::peer_from_path(b_path);

    std::vector<std::byte> msg(32);
    for (int i = 0; i < 32; ++i) msg[i] = static_cast<std::byte>(i);
    // Here needs a dest
    ASSERT_EQ(a.send_to(msg, b_peer), msg.size());

    std::array<std::byte, 128> buf{};
    // from will be filled with sou
    Peer from{};
    const auto n = b.recv_into(buf, from);

    ASSERT_EQ(n, msg.size());
    for (std::size_t i = 0; i < n; ++i) EXPECT_EQ(buf[i], msg[i]);
}