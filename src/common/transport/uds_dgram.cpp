#include "transport/uds_dgram.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <unistd.h>

namespace ts::transport {

static void throw_errno(const char* what) {
  throw std::runtime_error(std::string(what) + ": " + std::strerror(errno));
}

static sockaddr_un make_addr(const std::string& path) {
  if (path.size() >= sizeof(sockaddr_un::sun_path)) {
    throw std::runtime_error("UDS path too long: " + path);
  }
  sockaddr_un a{};
  a.sun_family = AF_UNIX;
  std::memcpy(a.sun_path, path.c_str(), path.size() + 1);
  return a;
}

Peer UdsDgramSocket::peer_from_path(const std::string& path) {
  Peer p{};
  p.addr = make_addr(path);
  p.len = static_cast<socklen_t>(sizeof(sockaddr_un));
  return p;
}

UdsDgramSocket::UdsDgramSocket(std::string local_path)
    : path_(std::move(local_path)) {
  fd_ = ::socket(AF_UNIX, SOCK_DGRAM, 0);
  if (fd_ < 0) throw_errno("socket");

  ::unlink(path_.c_str()); // safe even if missing

  sockaddr_un a = make_addr(path_);
  if (::bind(fd_, reinterpret_cast<sockaddr*>(&a), sizeof(a)) < 0) {
    throw_errno("bind");
  }
}

UdsDgramSocket::~UdsDgramSocket() {
  if (fd_ >= 0) ::close(fd_);
  if (!path_.empty()) ::unlink(path_.c_str());
}

UdsDgramSocket::UdsDgramSocket(UdsDgramSocket&& o) noexcept
    : fd_(o.fd_), path_(std::move(o.path_)) {
  o.fd_ = -1;
}

UdsDgramSocket& UdsDgramSocket::operator=(UdsDgramSocket&& o) noexcept {
  if (this == &o) return *this;
  if (fd_ >= 0) ::close(fd_);
  if (!path_.empty()) ::unlink(path_.c_str());
  fd_ = o.fd_;
  path_ = std::move(o.path_); // call string's move constructor
  o.fd_ = -1;
  return *this;
}

std::size_t UdsDgramSocket::recv_into(std::span<std::byte> buf, Peer& from) {
  from.len = sizeof(from.addr);
  const auto n = ::recvfrom(fd_,
                           buf.data(),
                           buf.size(),
                           0,
                           reinterpret_cast<sockaddr*>(&from.addr),
                           &from.len);
  if (n < 0) throw_errno("recvfrom");
  return static_cast<std::size_t>(n);
}

std::size_t UdsDgramSocket::send_to(std::span<const std::byte> data, const Peer& to) {
  const auto n = ::sendto(fd_,
                          data.data(),
                          data.size(),
                          0,
                          reinterpret_cast<const sockaddr*>(&to.addr),
                          to.len);
  if (n < 0) throw_errno("sendto");
  return static_cast<std::size_t>(n);
}

} // namespace ts::transport