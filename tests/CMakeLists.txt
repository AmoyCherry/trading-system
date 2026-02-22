# tests/CMakeLists.txt

# fetch google test
include(FetchContent)

FetchContent_Declare(
        gtest
        URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gtest)

include(GoogleTest)

# add executable test
add_executable(unit_tests
        test_engine_basic.cpp
)

# Linking ts_engine pulls in the "PUBLIC" include directories defined above
target_link_libraries(unit_tests PRIVATE
        ts_engine
        ts_common
        GTest::gtest_main
)

gtest_discover_tests(unit_tests)
