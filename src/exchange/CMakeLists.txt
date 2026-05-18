# Define a library target
add_library(exchange_gen)
# Add source files to this library target for other reference
target_sources(exchange_gen PRIVATE
        generator/scenarios.hpp
        generator/scenarios.cpp
)
# Export library for others to reference
target_include_directories(exchange_gen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Add dependencies to this library target
target_link_libraries(exchange_gen PUBLIC ts_common ts_engine)

# Define an executable library
add_executable(exchange_sim main.cpp)
# Add dependencies to this executable target
target_link_libraries(exchange_sim PRIVATE exchange_gen ts_common ts_engine)
