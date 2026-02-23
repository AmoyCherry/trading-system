#pragma once

#include "protocol/pipeline.hpp"
#include "time/clock.hpp"

// include your engine interface (adapt path to your project)
#include "engine.hpp"

#include <type_traits>
#include <variant>

namespace ts::engine {
// todo! why there is a class Engine
class Engine;
}

namespace ts::gw {

// Returns attribution for this request
struct GwMetrics {
    std::uint64_t total_ns{0};   // gateway handle() wall time
    std::uint64_t lob_ns{0};     // time spent inside engine call
    std::uint32_t event_count{0};
};

class InProcGateway {
public:
    explicit InProcGateway(ts::engine::Engine& eng) : eng_(&eng) {}

    // Templated sink avoids std::function overhead (nice for measurement)
    template <class Sink>
    GwMetrics handle(const ts::proto::ClientMsg& msg, Sink&& out) {
        const auto t0 = ts::time::now_ns();

        // Pre-gateway work would go here (validation, seq stamping, etc.)
        const auto t_lob_start = ts::time::now_ns();

        GwMetrics m{};
        auto counting_sink = [&](const ts::engine::Event& ev) {
            ++m.event_count;
            out(ev);
        };

        std::visit([&](auto&& inner) {
          using M = std::decay_t<decltype(inner)>;
          if constexpr (std::is_same_v<M, ts::proto::NewOrder>) {
            eng_->on_new(inner, counting_sink);
          } else if constexpr (std::is_same_v<M, ts::proto::Cancel>) {
            eng_->on_cancel(inner, counting_sink);
          }
        }, msg);

        const auto t_lob_end = ts::time::now_ns();
        const auto t1 = t_lob_end;

        m.lob_ns   = t_lob_end - t_lob_start;
        m.total_ns = t1 - t0;
        return m;
    }

private:
    ts::engine::Engine* eng_;
};

} // namespace ts::gw
