add_library(tradex_admin STATIC
    src/admin.cpp
)

# Platform-specific libraries
if(WINDOWS)
    set(ADMIN_LIBS ws2_32)
else()
    set(ADMIN_LIBS pthread)
endif()

target_include_directories(tradex_admin PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(tradex_admin
    tradex_framework
    tradex_order
    tradex_trade
    tradex_risk
    tradex_net
    ${ADMIN_LIBS}
)

target_compile_options(tradex_admin PRIVATE
    ${WARNING_FLAGS}
)