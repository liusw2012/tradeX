add_library(tradex_risk STATIC
    src/risk.cpp
)

target_include_directories(tradex_risk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_risk
    tradex_framework
    tradex_order
)

target_compile_options(tradex_risk PRIVATE
    ${WARNING_FLAGS}
)