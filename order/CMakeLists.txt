add_library(tradex_order STATIC
    src/order.cpp
    src/order_service.cpp
)

target_include_directories(tradex_order PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_order
    tradex_framework
)

target_compile_options(tradex_order PRIVATE
    ${WARNING_FLAGS}
)