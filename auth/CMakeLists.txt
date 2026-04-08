add_library(tradex_auth STATIC
    src/auth.cpp
)

target_include_directories(tradex_auth PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_auth
    tradex_framework
)

target_compile_options(tradex_auth PRIVATE
    ${WARNING_FLAGS}
)