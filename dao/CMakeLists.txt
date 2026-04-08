add_library(tradex_dao STATIC
    src/connection_pool.cpp
)

target_include_directories(tradex_dao PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_dao
    tradex_framework
)

target_compile_options(tradex_dao PRIVATE
    ${WARNING_FLAGS}
)