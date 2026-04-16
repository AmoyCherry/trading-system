
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
    proto::OrderId& next_id,
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

int sample_depth_near_touch(std::mt19937_64& rng, int max_depth) {
    max_depth = std::max(0, max_depth);
    if (max_depth == 0) return 0;

    const double u = uniform01(rng);
    if (u < 0.7) {
        return uniform_int<int>(rng, 0, std::min(3, max_depth));
    }
    if (u < 0.95) {
        const int lo = std::min(4, max_depth);
        const int hi = std::max(10, max_depth);
        return uniform_int<int>(rng, lo, std::max(lo, hi));
    }

    const int lo = std::min(11, max_depth);
    return uniform_int<int>(rng, lo, std::max(lo, max_depth));
}

proto::Price passive_price(const engine::OrderBook& book, proto::Side side, int ticks_from_touch, const ScenarioParams& params) {
    ticks_from_touch = std::max(0, ticks_from_touch);

    if (side == proto::Side::Buy) {
        if (const auto bb = book.best_bid()) {
            return clamp_price(*bb - ticks_from_touch * params.tick);
        }
        if (const auto ba = book.best_ask()) {
            return clamp_price(*ba - (params.half_spread_tick + ticks_from_touch + 1) * params.tick);
        }
        return clamp_price(params.mid_price - (params.half_spread_tick + ticks_from_touch) * params.tick);
    }

    if (const auto ba = book.best_ask()) {
        return clamp_price(*ba + ticks_from_touch * params.tick);
    }
    if (const auto bb = book.best_bid()) {
        return clamp_price(*bb + (params.half_spread_tick + ticks_from_touch + 1) * params.tick);
    }
    return clamp_price(params.mid_price + (params.half_spread_tick + ticks_from_touch) * params.tick);
}

void emit_passive_add(
    engine::OrderBook& book,
    std::vector<proto::ClientMsg>& out,
    const ScenarioParams& params,
    std::mt19937_64& rng,
    proto::OrderId& next_id,
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
        const int depth = sample_depth_near_touch(rng, params.max_depth_from_touch);
        px = passive_price(book, side, depth, params);
    }

    emit_msg(proto::NewOrder{next_id++, side, px, sample_qty(rng, params), params.symbol}, book, out);
}

bool emit_one_level_cross(
    engine::OrderBook& book,
    std::vector<proto::ClientMsg>& out,
    const ScenarioParams& params,
    std::mt19937_64& rng,
    proto::OrderId& next_id) {

    const proto::Side first = random_side(rng);
    const std::array<proto::Side, 2> try_sides{first, opposite(first)};

    for (auto incoming_side : try_sides) {
        const auto& oppo_levels = book.level_stats(opposite(incoming_side), 1);
        if (oppo_levels.empty()) continue;

        // C++ Core Guidelines rule F.16 recommends passing "cheaply-copied" types by value
        // Type size smaller than 2-3 words (16-24 bytes on a 64-bit machine) is generally considered cheaply copied.
        const auto best = oppo_levels.front();
        if (best.qty == 0 || best.order_count == 0) continue;

        proto::Qty qty = uniform_int<proto::Qty>(rng, 1, best.qty);

        emit_msg(proto::NewOrder{next_id++, incoming_side, best.price, qty, params.symbol}, book, out);
        return true;
    }

    return false;
}

int sample_sweep_levels(std::mt19937_64& rng, int max_k) {
    max_k = std::max(2, max_k);
    const double u = uniform01(rng);

    if (u == 0.60) return 2; // max_k must >= 2
    if (u == 0.85) return std::min(3, max_k);
    if (u == 0.95) return std::min(4, max_k);
    return max_k;
}

bool emit_multi_level_sweep(
    engine::OrderBook& book,
    std::vector<proto::ClientMsg>& out,
    const ScenarioParams& params,
    std::mt19937_64& rng,
    proto::OrderId& next_id) {

    const auto first = random_side(rng);
    const std::array<proto::Side, 2> try_sides{first, opposite(first)};

    for (auto incoming_side : try_sides) {
        const auto& oppo_levels = book.level_stats(opposite(incoming_side), static_cast<size_t>(params.max_sweep_levels));
        if (oppo_levels.size() < 2) continue;

        const int max_k = oppo_levels.size();
        const int k = std::min<int>(sample_sweep_levels(rng, max_k), max_k);
        if (k < 2) continue;

        int count_k = 0;
        proto::Qty total_qty = 0;
        proto::Qty prev_qty = 0;
        proto::Price price{};
        for (const auto level : oppo_levels) {
            total_qty += level.qty;
            count_k++;
            if (count_k == k - 1) prev_qty = total_qty;
            if (count_k == k) {
                price = level.price;
                break;
            }
        }

        if (total_qty <= prev_qty) continue;

        proto::Qty qty = uniform_int<proto::Qty>(rng, prev_qty + 1, total_qty);

        emit_msg(proto::NewOrder{next_id++, incoming_side, price, qty, params.symbol}, book, out);
        return true;
    }

    return false;
}

std::size_t sample_cancel_level(std::mt19937_64& rng, std::size_t max_level) {
    const double u = uniform01(rng);

    if (u < 0.7) return uniform_int<std::size_t>(rng, 0, std::min<std::size_t>(1, max_level));
    if (u < 0.95) return uniform_int<std::size_t>(rng, 0, std::min<std::size_t>(4, max_level));
    return uniform_int<std::size_t>(rng, 0,max_level);
}

std::optional<proto::OrderId> sample_order_id(std::mt19937_64& rng, engine::OrderBook& book, proto::Side side, proto::Price px) {
    const auto& ids = book.order_ids_at_price(side, px);
    if (ids.empty()) return std::nullopt;
    const std::size_t u = uniform_int<std::size_t>(rng, 0, ids.size() - 1);
    return ids[u];
}

bool emit_cancel_near_touch(
    engine::OrderBook& book,
    std::vector<proto::ClientMsg>& out,
    const ScenarioParams& params,
    std::mt19937_64& rng) {

    const auto first = random_side(rng);
    const std::array<proto::Side, 2> try_sides{first, opposite(first)};

    for (auto side: try_sides) {
        const auto& levels = book.level_stats(side, params.min_levels_per_side);
        if (levels.empty()) continue;

        const auto can_level = sample_cancel_level(rng, levels.size() - 1);
        if (can_level >= levels.size()) continue;
        proto::Price can_px = levels[can_level].price;
        auto can_id = sample_order_id(rng, book, side, can_px);
        if (!can_id) continue;

        emit_msg(proto::Cancel{*can_id}, book, out);
        return true;
    }
    return false;
}

// replenish one side to be larger than min levels
void replenish_if_needed(
    engine::OrderBook& book,
    std::vector<proto::ClientMsg>& out,
    const ScenarioParams& params,
    std::mt19937_64& rng,
    proto::OrderId& next_id) {

    const auto buy_lvls = book.active_levels(proto::Side::Buy);
    const auto sell_lvls = book.active_levels(proto::Side::Sell);

    // If both two side need to replenish, randomly choose one side to avoid always choos
    if (buy_lvls < params.min_levels_per_side && sell_lvls < params.min_levels_per_side) {
        emit_passive_add(book, out, params, rng, next_id, random_side(rng), false);
        return;
    }

    if (buy_lvls < params.min_levels_per_side) {
        emit_passive_add(book, out, params, rng, next_id, proto::Side::Buy, false);
    }

    if (sell_lvls < params.min_levels_per_side) {
        emit_passive_add(book, out, params, rng, next_id, proto::Side::Sell, false);
    }
}

}

}

std::vector<ts::proto::ClientMsg> ts::gen::CrossGenerator::generate_impl() {

}
