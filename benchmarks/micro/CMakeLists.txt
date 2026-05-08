# Define an executable target
add_executable(microbench bm_matching.cpp)

target_compile_options(microbench PRIVATE -fno-omit-frame-pointer)

# Add dependencies to the target
target_link_libraries(microbench PRIVATE
        benchmark::benchmark
        ts_engine
        ts_common
)