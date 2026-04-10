#pragma once

#include <cstdint>
#include <vector>

#include "../../engine/engine.hpp"
#include "protocol/pipeline.hpp"


namespace ts::gen {

namespace proto = ts::proto;

struct ScenarioParams {
    std::uint64_t seed = 0xC0FFEEULL;

    proto::Symbol symbol = 0;
    proto::Price mid_price = 10'000;
    proto::Price tick = 1; // min step size of price

    std::uint32_t seed_levels_per_side = 12;
    std::uint32_t seed_orders_per_level = 3;

    // If one side falls below this depth, force replenishment.
    std::uint32_t min_levels_per_side = 6;

    // Used for passive placement distributions.
    int max_depth_from_touch = 15;

    proto::Qty min_qty = 1;
    proto::Qty max_qty = 12;

    double p_inside_spread = 0.08;

    double p_passive = 0.50;
    double p_one_level = 0.30;
    double p_sweep = 0.20;
    double max_sweep_levels = min_levels_per_side - 1;

    proto::Price half_spread_tick = 1;
};

template <typename T>
concept GenerateImplemented = requires(T a) {
    { a.generate_impl() } -> std::same_as<std::vector<proto::ClientMsg>>;
};

template <typename Derived>
class GeneratorBase {
private:
    std::uint32_t msg_count = 0;
    ScenarioParams sp{};
public:
    std::vector<proto::ClientMsg> generate() requires GenerateImplemented<Derived> {
        return static_cast<Derived*>(this)->generate_impl();
    }
};

class CrossGenerator : GeneratorBase<CrossGenerator> {
    std::vector<proto::ClientMsg> generate_impl();
};

class AddOnlyGenerator : GeneratorBase<AddOnlyGenerator> {
    std::vector<proto::ClientMsg> generate_impl();
};

class CancelHeavyGenerator : GeneratorBase<CancelHeavyGenerator> {
    std::vector<proto::ClientMsg> generate_impl();
};

}
