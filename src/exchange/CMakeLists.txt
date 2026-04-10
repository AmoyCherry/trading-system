add_executable(exchange_sim main.cpp
        generator/scenarios.hpp
        generator/scenarios.cpp)
target_link_libraries(exchange_sim PRIVATE ts_common)
