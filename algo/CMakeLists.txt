add_library(tradex_algo STATIC
    src/algo.cpp
)

target_include_directories(tradex_algo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_algo
    tradex_framework
    tradex_order
)

target_compile_options(tradex_algo PRIVATE
    ${WARNING_FLAGS}
)