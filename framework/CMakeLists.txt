add_library(tradex_framework STATIC
    src/log.cpp
    src/config.cpp
    src/thread_pool.cpp
    src/status.cpp
)

target_include_directories(tradex_framework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(tradex_framework PRIVATE
    ${WARNING_FLAGS}
)