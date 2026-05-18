# Define an executable target
add_executable(inproc_runner inproc_runner.cpp)
# Include other libs to the target to meet the target's dependencies
target_link_libraries(inproc_runner PRIVATE
        ts_common
        ts_engine
        ts_gw
        exchange_gen
)

# Define an executable target
add_executable(replay_ref replay_ref.cpp)
# Include other libs to the target to meet the target's dependencies
target_link_libraries(replay_ref PRIVATE
        ts_engine
        exchange_gen
)
