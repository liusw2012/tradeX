add_library(tradex_market STATIC
    src/market.cpp
)

target_include_directories(tradex_market PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_market
    tradex_framework
)

target_compile_options(tradex_market PRIVATE
    ${WARNING_FLAGS}
)