add_library(ts_engine)
target_sources(ts_engine PRIVATE
        engine.cpp
        book/order_book.cpp
)
target_include_directories(ts_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ts_engine PUBLIC ts_common)
target_compile_features(ts_engine PUBLIC cxx_std_20)
