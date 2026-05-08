# Define a library target
add_library(ts_common)
# Add source files to the library
target_sources(ts_common PRIVATE
        # Listing hpp files just cosmetic
        # Compiler doesnt compile headers directly; hpp files get pulled via #include from cpp files
        transport/uds_dgram.hpp
        transport/uds_dgram.cpp
        protocol/messages.hpp
        protocol/wire.hpp
        protocol/pipeline.hpp
        time/clock.cpp
        stats/percentiles.hpp
        util/affinity.cpp
        util/cli.cpp
)
# Export the library
target_include_directories(ts_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(ts_common PUBLIC cxx_std_20)
