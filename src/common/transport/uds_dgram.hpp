#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include <sys/socket.h>
#include <sys/un.h>

namespace ts::transport {

  struct Peer {
    sockaddr_un addr{};
    socklen_t len{};
  };

  class UdsDgramSocket {
  public:
    explicit UdsDgramSocket(std::string local_path);
    ~UdsDgramSocket();

    UdsDgramSocket(const UdsDgramSocket&) = delete;
    UdsDgramSocket& operator=(const UdsDgramSocket&) = delete;

    UdsDgramSocket(UdsDgramSocket&&) noexcept;
    UdsDgramSocket& operator=(UdsDgramSocket&&) noexcept;

    int fd() const { return fd_; }
    const std::string& path() const { return path_; }

    static Peer peer_from_path(const std::string& path);

    // recvfrom: returns bytes read + peer address
    ssize_t recv_into(std::span<std::byte> buf, Peer& from);

    // sendto
    std::size_t send_to(std::span<const std::byte> data, const Peer& to);

  private:
    int fd_{-1};
    std::string path_;
  };

} // namespace ts::transport