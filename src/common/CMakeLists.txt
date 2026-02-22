add_library(ts_common)
target_sources(ts_common PRIVATE
        protocol/messages.hpp
        protocol/wire.hpp
        time/clock.cpp
        stats/percentiles.cpp
        util/affinity.cpp
        util/cli.cpp
)
target_include_directories(ts_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ts_common PUBLIC cxx_std_20)
