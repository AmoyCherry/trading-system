#pragma once

#include "protocol/messages.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <variant>

#ifdef __APPLE__
#include <ibkern/OSByteOrder.h>

#define htole16(x) OSSwapHostToLittleInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define htobe16(x) OSSwapHostToBigInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)

#define htole32(x) OSSwapHostToLittleInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)

#define htole64(x) OSSwapHostToLittleInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)

#else
#include <endian.h> // htoleXX, leXXtoh
#endif

namespace ts::wire {

namespace proto = ts::proto;

constexpr std::uint16_t kWireVersion = 1;
constexpr std::size_t   kMaxFrameSize = 256;
constexpr std::size_t   kHeaderSize = 16;

enum class MsgType : std::uint16_t {
  NewOrder    = 1,
  Cancel      = 2,
  OrderAck    = 3,
  CancelAck   = 4,
  Reject      = 5,
  Fill        = 6,
  EndOfReplay = 7,
};

using WireMsg = std::variant<
    proto::NewOrder, proto::Cancel,
    proto::OrderAck, proto::CancelAck, proto::Reject, proto::Fill,
    proto::EndOfReplay
  >;

struct Frame {
  std::array<std::byte, kMaxFrameSize> buf{};
  std::uint32_t len{0};

  std::span<const std::byte> bytes_view() const { return {buf.data(), len}; }
  std::span<std::byte> writable() { return {buf.data(), buf.size()}; }
};

struct Decoded {
  // Header
  std::uint16_t version{};
  MsgType type{};
  std::uint32_t length{};
  std::uint64_t seq{};
  // Payload
  WireMsg msg{};
};

// ---- helpers: write/read LE primitives safely ----

inline std::byte* write_u16(std::byte* p, std::uint16_t v) {
  v = htole16(v);
  std::memcpy(p, &v, sizeof(v));
  return p + sizeof(v);
}

inline std::byte* write_u32(std::byte* p, std::uint32_t v) {
  v = htole32(v);
  std::memcpy(p, &v, sizeof(v));
  return p + sizeof(v);
}

inline std::byte* write_u64(std::byte* p, std::uint64_t v) {
  v = htole64(v);
  std::memcpy(p, &v, sizeof(v));
  return p + sizeof(v);
}

inline std::byte* write_i32(std::byte* p, std::int32_t v) {
  auto u = htole32(static_cast<std::uint32_t>(v));
  std::memcpy(p, &u, sizeof(u));
  return p + sizeof(u);
}

inline bool read_u16(const std::byte*& p, const std::byte* end, std::uint16_t& out) {
  if (p + 2 > end) return false;
  std::uint16_t v{};
  std::memcpy(&v, p, 2);
  out = le16toh(v);
  p += 2;
  return true;
}

inline bool read_u32(const std::byte*& p, const std::byte* end, std::uint32_t& out) {
  if (p + 4 > end) return false;
  std::uint32_t v{};
  std::memcpy(&v, p, 4);
  out = le32toh(v);
  p += 4;
  return true;
}

inline bool read_u64(const std::byte*& p, const std::byte* end, std::uint64_t& out) {
  if (p + 8 > end) return false;
  std::uint64_t v{};
  std::memcpy(&v, p, 8);
  out = le64toh(v);
  p += 8;
  return true;
}

inline bool read_i32(const std::byte*& p, const std::byte* end, std::int32_t& out) {
  if (p + 4 > end) return false;
  std::uint32_t u{};
  std::memcpy(&u, p, 4);
  out = static_cast<std::int32_t>(le32toh(u));
  p += 4;
  return true;
}

inline bool read_u8(const std::byte*& p, const std::byte* end, std::uint8_t& out) {
  if (p + 1 > end) return false;
  out = static_cast<std::uint8_t>(*p);
  p += 1;
  return true;
}

inline std::byte* write_u8(std::byte* p, std::uint8_t v) {
  *p = static_cast<std::byte>(v);
  return p + 1;
}

// ---- encode: header + payload ----

inline void write_header(Frame& f, MsgType t, std::uint32_t length, std::uint64_t seq) {
  f.len = length;
  std::byte* p = f.buf.data();
  p = write_u16(p, kWireVersion);
  p = write_u16(p, static_cast<std::uint16_t>(t)); // msg type has fixed 2 bytes offset in Header
  p = write_u32(p, length);
  p = write_u64(p, seq);
}

inline bool encode(const proto::NewOrder& m, std::uint64_t seq, Frame& out) {
  // payload: id(u64), side(u8), price(i32), qty(i32), symbol(u32)
  constexpr std::uint32_t payload = 8 + 1 + 4 + 4 + 4;
  const std::uint32_t total = static_cast<std::uint32_t>(kHeaderSize + payload);
  if (total > kMaxFrameSize) return false;

  write_header(out, MsgType::NewOrder, total, seq);
  std::byte* p = out.buf.data() + kHeaderSize;
  p = write_u64(p, m.id);
  p = write_u8(p, static_cast<std::uint8_t>(m.side));
  p = write_i32(p, m.price);
  p = write_i32(p, m.qty);
  p = write_u32(p, m.symbol);
  return true;
}

inline bool encode(const proto::Cancel& m, std::uint64_t seq, Frame& out) {
  constexpr std::uint32_t payload = 8;
  const std::uint32_t total = static_cast<std::uint32_t>(kHeaderSize + payload);
  if (total > kMaxFrameSize) return false;

  write_header(out, MsgType::Cancel, total, seq);
  std::byte* p = out.buf.data() + kHeaderSize;
  p = write_u64(p, m.id);
  return true;
}

inline bool encode(const proto::OrderAck& m, std::uint64_t seq, Frame& out) {
  constexpr std::uint32_t payload = 8;
  const std::uint32_t total = static_cast<std::uint32_t>(kHeaderSize + payload);
  if (total > kMaxFrameSize) return false;

  write_header(out, MsgType::OrderAck, total, seq);
  std::byte* p = out.buf.data() + kHeaderSize;
  p = write_u64(p, m.id);
  return true;
}

inline bool encode(const proto::CancelAck& m, std::uint64_t seq, Frame& out) {
  constexpr std::uint32_t payload = 8;
  const std::uint32_t total = static_cast<std::uint32_t>(kHeaderSize + payload);
  if (total > kMaxFrameSize) return false;

  write_header(out, MsgType::CancelAck, total, seq);
  std::byte* p = out.buf.data() + kHeaderSize;
  p = write_u64(p, m.id);
  return true;
}

inline bool encode(const proto::Reject& m, std::uint64_t seq, Frame& out) {
  // id(u64), reason(u8)
  constexpr std::uint32_t payload = 8 + 1;
  const std::uint32_t total = static_cast<std::uint32_t>(kHeaderSize + payload);
  if (total > kMaxFrameSize) return false;

  write_header(out, MsgType::Reject, total, seq);
  std::byte* p = out.buf.data() + kHeaderSize;
  p = write_u64(p, m.id);
  p = write_u8(p, static_cast<std::uint8_t>(m.reason));
  return true;
}

inline bool encode(const proto::Fill& m, std::uint64_t seq, Frame& out) {
  // id(u64), contra(u64), price(i32), qty(i32), is_taker(u8)
  constexpr std::uint32_t payload = 8 + 8 + 4 + 4 + 1;
  const std::uint32_t total = static_cast<std::uint32_t>(kHeaderSize + payload);
  if (total > kMaxFrameSize) return false;

  write_header(out, MsgType::Fill, total, seq);
  std::byte* p = out.buf.data() + kHeaderSize;
  p = write_u64(p, m.id);
  p = write_u64(p, m.contra_id);
  p = write_i32(p, m.price);
  p = write_i32(p, m.qty);
  p = write_u8(p, static_cast<std::uint8_t>(m.is_taker ? 1 : 0));
  return true;
}

inline bool encode(const proto::EndOfReplay& m, std::uint64_t seq, Frame& out) {
  constexpr std::uint32_t payload = 0;
  const std::uint32_t total = static_cast<std::uint32_t>(kHeaderSize + payload);
  if (total > kMaxFrameSize) return false;

  write_header(out, MsgType::EndOfReplay, total, seq);
  return true;
}

// Convenience: encode a WireMsg variant and draw the Frame
inline bool encode(const WireMsg& msg, std::uint64_t seq, Frame& out) {
  return std::visit([&](auto&& inner) { return encode(inner, seq, out); }, msg);
}

// ---- decode ----

inline std::optional<Decoded> decode(std::span<const std::byte> bytes) {
  if (bytes.size() < kHeaderSize) return std::nullopt;
  if (bytes.size() > kMaxFrameSize) return std::nullopt;

  const std::byte* p = bytes.data();
  const std::byte* end = bytes.data() + bytes.size();

  Decoded d{};
  std::uint16_t type_u16{};
  if (!read_u16(p, end, d.version)) return std::nullopt;
  if (!read_u16(p, end, type_u16)) return std::nullopt;
  if (!read_u32(p, end, d.length)) return std::nullopt;
  if (!read_u64(p, end, d.seq)) return std::nullopt;

  d.type = static_cast<MsgType>(type_u16);

  if (d.version != kWireVersion) return std::nullopt;
  if (d.length != bytes.size()) return std::nullopt; // strict framing

  // payload begins at bytes[kHeaderSize]
  p = bytes.data() + kHeaderSize;

  switch (d.type) {
    case MsgType::NewOrder: {
      proto::NewOrder m{};
      std::uint8_t side{};
      if (!read_u64(p, end, m.id)) return std::nullopt;
      if (!read_u8(p, end, side)) return std::nullopt;
      m.side = static_cast<proto::Side>(side);
      if (!read_i32(p, end, m.price)) return std::nullopt;
      if (!read_i32(p, end, m.qty)) return std::nullopt;
      if (!read_u32(p, end, m.symbol)) return std::nullopt;
      d.msg = m;
      return d;
    }
    case MsgType::Cancel: {
      proto::Cancel m{};
      if (!read_u64(p, end, m.id)) return std::nullopt;
      d.msg = m;
      return d;
    }
    case MsgType::OrderAck: {
      proto::OrderAck m{};
      if (!read_u64(p, end, m.id)) return std::nullopt;
      d.msg = m;
      return d;
    }
    case MsgType::CancelAck: {
      proto::CancelAck m{};
      if (!read_u64(p, end, m.id)) return std::nullopt;
      d.msg = m;
      return d;
    }
    case MsgType::Reject: {
      proto::Reject m{};
      std::uint8_t reason{};
      if (!read_u64(p, end, m.id)) return std::nullopt;
      if (!read_u8(p, end, reason)) return std::nullopt;
      m.reason = static_cast<proto::RejectReason>(reason);
      d.msg = m;
      return d;
    }
    case MsgType::Fill: {
      proto::Fill m{};
      std::uint8_t taker{};
      if (!read_u64(p, end, m.id)) return std::nullopt;
      if (!read_u64(p, end, m.contra_id)) return std::nullopt;
      if (!read_i32(p, end, m.price)) return std::nullopt;
      if (!read_i32(p, end, m.qty)) return std::nullopt;
      if (!read_u8(p, end, taker)) return std::nullopt;
      m.is_taker = (taker != 0);
      d.msg = m;
      return d;
    }
    case MsgType::EndOfReplay: {
      d.msg = proto::EndOfReplay{};
      return d;
    }
    default:
      return std::nullopt;
  }
}

inline std::optional<MsgType> decode_type(std::span<const std::byte> bytes) {
  if (bytes.size() < kHeaderSize) return std::nullopt;
  if (bytes.size() > kMaxFrameSize) return std::nullopt;

  const std::byte* p = bytes.data();
  const std::byte* end = bytes.data() + bytes.size();
  p += 2;
  std::uint16_t type_u16{};
  if (!read_u16(p, end, type_u16)) return std::nullopt;

  auto type = static_cast<MsgType>(type_u16);
  return type;
}

} // namespace ts::wire
