# Define a executable target
add_executable(lobd main.cpp)
# Add dependencies to this target
target_link_libraries(lobd PRIVATE ts_engine ts_common)
