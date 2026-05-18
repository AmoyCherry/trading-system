#include <string>
#include <cstdint>
#include <iostream>

#include "book_summary.hpp"
#include "generator/scenarios.hpp"
#include "book/order_book.hpp"


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

struct ReplayCounters {
    std::uint64_t acks = 0;
    std::uint64_t cancel_acks = 0;
    std::uint64_t rejects = 0;
    std::uint64_t fills = 0;

    // todo: verify - It's better to use visitor when it handles a stream; it's better to use holds_alternative directly if just a call
    void on_event(const ts::engine::Event& e) {
        std::visit([&](auto&& inner) {
            using M = std::decay_t<decltype((inner))>;
            if constexpr (std::is_same_v<M, ts::proto::OrderAck>) ++acks;
            else if constexpr (std::is_same_v<M, ts::proto::CancelAck>) ++cancel_acks;
            else if constexpr (std::is_same_v<M, ts::proto::Reject>) ++rejects;
            else if constexpr (std::is_same_v<M, ts::proto::Fill>) ++fills;
        }, e);
    }
};

void print_result(const ReplayCounters& ctr, const ts::engine::BookSummary bs, std::uint64_t msg_hash) {
    std::cout << "RESULT" << "\n"
            << " acks=" << ctr.acks << "\n"
            << " cancel_acks=" << ctr.cancel_acks << "\n"
            << " rejects=" << ctr.rejects << "\n"
            << " fills=" << ctr.fills << "\n"
            << " live_orders=" << bs.live_orders << "\n"
            << " best_bid=" << ts::engine::price_or_na(bs.best_bid) << "\n"
            << " best_ask=" << ts::engine::price_or_na(bs.best_ask)<< "\n"
            << " state_hash=" << bs.state_hash<< "\n"
            << " msg_hash=" << msg_hash<< "\n"
            << "\n";
}

const uint64_t kFnvOffset = 14695981039346656037ull;
const uint64_t kFnvPrime = 1099511628211ull;

inline void hash_byte(std::uint64_t& h, std::uint8_t v) {
    h ^= v;
    h *= kFnvPrime;
}

inline void hash_u64(std::uint64_t& h, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        hash_byte(h, static_cast<std::uint8_t>(v & 0xffu));
        v >>= 8;
    }
}

inline void hash_u32(std::uint64_t& h, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        hash_byte(h, static_cast<std::uint8_t>(v & 0xffu));
        v >>= 8;
    }
}

inline void hash_i32(std::uint64_t& h, std::int32_t v) {
    hash_u32(h, static_cast<std::uint32_t>(v));
}

inline void hash_msg(std::uint64_t& h, const ts::proto::NewOrder& order) {
    hash_u64(h, order.id);
    hash_byte(h, static_cast<uint8_t>(order.side));
    hash_i32(h, order.price);
    hash_i32(h, order.qty);
    hash_u32(h, order.symbol);
}

inline void hash_msg(std::uint64_t& h, const ts::proto::Cancel& order) {
    hash_u64(h, order.id);
}

}

int main(int argc, char** argv) {
    const std::string scenario_name = arg(argc, argv, "--scenario", "cross");
    const std::uint64_t total_msgs = arg_u64(argc, argv, "--n", 200'000);

    const auto kind = ts::gen::parse_kind(scenario_name);
    const auto& stream = ts::gen::make(kind, total_msgs);

    std::uint64_t msg_hash = kFnvOffset;
    hash_u64(msg_hash, static_cast<uint64_t>(stream.size()));

    ts::engine::Engine engine;
    ReplayCounters ctr;
    ts::engine::EventSink out = [&](const ts::engine::Event& e) { ctr.on_event(e); };

    for (const auto& msg : stream) {
        std::visit([&](auto&& inner) {
            // todo: check - the final compilation result is identical to ofunction overloads??
            using M = std::decay_t<decltype(inner)>;
            if constexpr (std::is_same_v<M, ts::proto::NewOrder>) {
                engine.on_new(inner, out);
                hash_msg(msg_hash, inner);
            } else if constexpr (std::is_same_v<M, ts::proto::Cancel>) {
                engine.on_cancel(inner, out);
                hash_msg(msg_hash, inner);
            }
        }, msg);
    }

    print_result(ctr, engine.summary(), msg_hash);
    return 0;
}