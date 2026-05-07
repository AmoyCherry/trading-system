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
    uint32_t max_sweep_levels = min_levels_per_side - 1;

    double p_cancel = 0.65;

    proto::Price half_spread_tick = 1;
};

// template <typename T>
// concept GenerateImplemented = requires(T a) {
//     { a.generate_impl() } -> std::same_as<std::vector<proto::ClientMsg>>;
// };

template <typename Derived>
class GeneratorBase {
    friend Derived; // NOTE 1: use friend or protected to allow Derived can access to private members
private:
    std::vector<proto::ClientMsg> out{};
    engine::OrderBook book{};
    proto::OrderId next_id = 1;
public:
    // std::vector<proto::ClientMsg> generate() requires GenerateImplemented<Derived> {
    //     return static_cast<Derived*>(this)->generate_impl();
    // }

    std::vector<proto::ClientMsg> generate(std::size_t total_msgs_, const ScenarioParams& params_) {
        out.clear(); // memory retention NOTE: if out reserved a large size before, now it won't shrink the capacity by reserving a small size.
        book = engine::OrderBook{};
        next_id = 1;

        if (total_msgs_ == 0) return out;

        out.reserve(total_msgs_);
        std::mt19937_64 rng(params_.seed);

        static_cast<Derived*>(this)->generate_impl(total_msgs_, params_, rng);

        return out;
    }
};

class CrossGenerator : public GeneratorBase<CrossGenerator> { // NOTE 2: declare public inheritance to allow the base class's method `generate()` can be accessed from Derived
// public: NOTE 3: if we want to evaluate `generate_impl` by concept, we need to make it public; if we more want to keep `generate_impl` private, we need to drop the concept, and declare the base class as friend
    friend class GeneratorBase<CrossGenerator>;
private:
    void generate_impl(std::size_t total_msgs_, const ScenarioParams& params_, std::mt19937_64& rng);
};

class AddOnlyGenerator : public GeneratorBase<AddOnlyGenerator> {
    friend class GeneratorBase<AddOnlyGenerator>;
private:
    void generate_impl(std::size_t total_msgs_, const ScenarioParams& params_, std::mt19937_64& rng);
};

class CancelHeavyGenerator : public GeneratorBase<CancelHeavyGenerator> {
    friend class GeneratorBase<CancelHeavyGenerator>;
private:
    void generate_impl(std::size_t total_msgs_, const ScenarioParams& params_, std::mt19937_64& rng);
};

}
