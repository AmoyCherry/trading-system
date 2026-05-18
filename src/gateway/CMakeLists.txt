# Define a library target
add_library(ts_gw)
# Add source files to the library for other to reference
target_sources(ts_gw PRIVATE
        inproc_gateway.hpp
)
# Export the library
target_include_directories(ts_gw PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Include other libs to the target to meet the target's dependencies
target_link_libraries(ts_gw PUBLIC ts_common ts_engine)

# Define an executable target
add_executable(gateway main.cpp)
# Add dependencies to the executable target
target_link_libraries(gateway PRIVATE ts_common ts_engine)