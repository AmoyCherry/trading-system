
#include <vector>
#include <random>

#include "scenarios.hpp"

#include <stdexcept>

namespace ts::gen {

namespace proto = ts::proto;
namespace engine = ts::engine;

namespace {

const engine::EventSink NoopSink = [](const engine::Event&) {};

template <class Int>
Int uniform_int(std::mt19937_64& rng, Int lo, Int hi) {
    if (hi < lo) std::swap(lo, hi);
    std::uniform_int_distribution<long long> dist(
        static_cast<long long>(lo), static_cast<long long>(hi));
    return static_cast<Int>(dist(rng));
}

double uniform01(std::mt19937_64& rng) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

bool bernoulli(std::mt19937_64& rng, double p) {
    std::bernoulli_distribution dist(p);
    return dist(rng);
}

proto::Side opposite(proto::Side side) {
    return (side == proto::Side::Buy) ? proto::Side::Sell : proto::Side::Buy;
}

proto::Side random_side(std::mt19937_64& rng) {
    return bernoulli(rng, 0.5) ? proto::Side::Buy : proto::Side::Sell;
}

proto::Qty sample_qty(std::mt19937_64& rng, const ScenarioParams& p) {
    const int lo = p.min_qty;
    const int hi = p.max_qty;

    const double u = uniform01(rng);
    if (u < 0.80) {
        return uniform_int<proto::Qty>(rng, lo, std::min<int>(hi, 4));
    }
    if (u < 0.95) {
        const int a = std::min<int>(std::max<int>(lo, 5), hi);
        const int b = std::min<int>(hi, 8);
        return uniform_int<proto::Qty>(rng, a, std::max(a, b));
    }

    const int a = std::min<int>(std::max<int>(lo, 9), hi);
    return uniform_int<proto::Qty>(rng, a, std::max(a, hi));
}


void emit_msg(const proto::NewOrder& msg, engine::OrderBook& book, std::vector<proto::ClientMsg>& out) {
    out.emplace_back(msg);
    book.on_new(msg, NoopSink);
}

// Cancel is smaller than 16 bytes and will be optimized to be stored in registers directly, which can be faster than passing by reference
void emit_msg(proto::Cancel msg, engine::OrderBook& book, std::vector<proto::ClientMsg>& out) {
    out.emplace_back(msg);
    book.on_cancel(msg, NoopSink);
}

void seed_book(
    engine::OrderBook& book,
    std::vector<proto::ClientMsg>& out,
    const ScenarioParams& params,
    std::mt19937_64& rng,
    proto::OrderId next_id,
    std::size_t total_msgs) {

    for (int level = 0; level < params.seed_levels_per_side && out.size() < total_msgs; ++level) {
        proto::Price bid_price = params.mid_price - (params.half_spread_tick + level) * params.tick;
        proto::Price ask_price = params.mid_price + (params.half_spread_tick + level) * params.tick;

        for (int j = 0; j < params.seed_orders_per_level && out.size() < total_msgs; ++j) {
            emit_msg(proto::NewOrder{next_id++, proto::Side::Buy, bid_price, sample_qty(rng, params), params.symbol},
                book, out);

            emit_msg(proto::NewOrder{next_id++, proto::Side::Sell, ask_price, sample_qty(rng, params), params.symbol},
                book, out);
        }
    }
}

bool can_place_inside(engine::OrderBook& book, const ScenarioParams& params) {
    auto bd = book.best_bid();
    auto ba = book.best_ask();
    if (ba && bd) {
        return *ba - *bd >= 2 * params.tick;
    }
    return false;
}

// todo! why I need this?
proto::Price clamp_price(proto::Price px) {
    return std::max<proto::Price>(1, px);
}

proto::Price inside_spread_price(const engine::OrderBook& book,
                                 proto::Side side,
                                 const ScenarioParams& p) {
    const auto bb = book.best_bid();
    const auto ba = book.best_ask();
    if (!bb || !ba) {
        throw std::runtime_error("Try to generate inside-spread price while one Side is empty. It should be handled by other procedures rather than the inside-spread scenario");
    }

    if (side == proto::Side::Buy) {
        return clamp_price(*bb + p.tick);
    }
    return clamp_price(*ba - p.tick);
}

void emit_passive_add(
    engine::OrderBook& book,
    std::vector<proto::ClientMsg>& out,
    const ScenarioParams& params,
    std::mt19937_64& rng,
    proto::OrderId next_id,
    std::size_t total_msgs,
    std::optional<proto::Side> forced = std::nullopt,
    bool allowed_inside_spread = false) {

    // construct side, px, order -> emit

    const auto side = forced ? *forced : random_side(rng);

    proto::Price px{};
    if (allowed_inside_spread &&
        can_place_inside(book, params) &&
        bernoulli(rng, params.p_inside_spread)) {
        px = inside_spread_price(book, side, params);
    } else {

    }

    emit_msg(proto::NewOrder{next_id++, side, px, sample_qty(rng, params), params.symbol}, book, out);
}

}

}

std::vector<ts::proto::ClientMsg> ts::gen::CrossGenerator::generate_impl() {

}
