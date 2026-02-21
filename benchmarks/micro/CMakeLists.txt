# 1. Define the executable first
add_executable(microbench bm_matching.cpp)

# 2. Now you can set options for it
target_compile_options(microbench PRIVATE -fno-omit-frame-pointer)

# 3. Link your libraries
target_link_libraries(microbench PRIVATE
        benchmark::benchmark
        ts_engine
        ts_common
)