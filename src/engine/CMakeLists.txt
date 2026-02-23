# Define library
add_library(ts_engine)
# Add source files to the library
target_sources(ts_engine PRIVATE
        engine.cpp
        engine.hpp
        book/order_book.cpp
)
# Export the library
target_include_directories(ts_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Include other libs to the target to meet the target's dependencies
target_link_libraries(ts_engine PUBLIC ts_common)

target_compile_features(ts_engine PUBLIC cxx_std_20)
