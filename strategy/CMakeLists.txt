add_library(tradex_strategy STATIC
    src/strategy.cpp
)

target_include_directories(tradex_strategy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_strategy
    tradex_framework
    tradex_order
    tradex_market
)

target_compile_options(tradex_strategy PRIVATE
    ${WARNING_FLAGS}
)