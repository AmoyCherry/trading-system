add_executable(e2e_runner runner/main.cpp)
target_link_libraries(e2e_runner PRIVATE ts_common)

# Define the executable
add_executable(inproc_runner inproc_runner.cpp)
# Include other libs to the target to meet the target's dependencies
target_link_libraries(inproc_runner PRIVATE
        ts_common
        ts_engine
        ts_gw
)
