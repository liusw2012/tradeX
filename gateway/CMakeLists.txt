add_library(tradex_gateway STATIC
    src/gateway.cpp
)

target_include_directories(tradex_gateway PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_gateway
    tradex_framework
    tradex_order
)

target_compile_options(tradex_gateway PRIVATE
    ${WARNING_FLAGS}
)