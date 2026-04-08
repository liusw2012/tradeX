add_library(tradex_trade STATIC
    src/trade.cpp
)

target_include_directories(tradex_trade PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_trade
    tradex_framework
    tradex_order
)

target_compile_options(tradex_trade PRIVATE
    ${WARNING_FLAGS}
)