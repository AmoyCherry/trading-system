# Define library
add_library(ts_gw)
# Add source files to the library
target_sources(ts_gw PRIVATE
        inproc_gateway.hpp
        # todo! why there must be a cpp file to reference to the hpp
        main.cpp
)
# Export the library
target_include_directories(ts_gw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Include other libs to the target to meet the target's dependencies
target_link_libraries(ts_gw PUBLIC ts_common ts_engine)

# Define the executable
add_executable(gateway main.cpp)
# gateway is another target different with ts_gw, so it also needs to include its dep
target_link_libraries(gateway PRIVATE ts_common ts_engine)