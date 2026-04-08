cmake_minimum_required(VERSION 3.10)

# CLI client executable
add_executable(tradeX_cli
    cli_main.cpp
    cli_commands.cpp
)

# Platform detection - use WINDOWS variable from parent
if(WINDOWS)
    set(PLATFORM_LIBS ws2_32)
else()
    set(PLATFORM_LIBS pthread)
endif()

target_link_libraries(tradeX_cli
    tradex_framework
    tradex_order
    tradex_trade
    tradex_risk
    tradex_market
    tradex_net
    ${PLATFORM_LIBS}
)

target_include_directories(tradeX_cli PRIVATE
    ${CMAKE_SOURCE_DIR}/framework/include
    ${CMAKE_SOURCE_DIR}/order/include
    ${CMAKE_SOURCE_DIR}/trade/include
    ${CMAKE_SOURCE_DIR}/risk/include
    ${CMAKE_SOURCE_DIR}/market/include
    ${CMAKE_SOURCE_DIR}/net/include
    ${CMAKE_SOURCE_DIR}/thirdparty/include
)