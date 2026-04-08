add_library(tradex_task STATIC
    src/task.cpp
)

target_include_directories(tradex_task PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_task
    tradex_framework
)

target_compile_options(tradex_task PRIVATE
    ${WARNING_FLAGS}
)