# Unit tests configuration
cmake_minimum_required(VERSION 3.10)

# Add test executable
add_executable(tradeX_test
    test_main.cpp
    test_order.cpp
    test_config.cpp
)

target_link_libraries(tradeX_test
    tradex_framework
    tradex_order
    tradex_trade
    tradex_risk
)

target_include_directories(tradeX_test PRIVATE
    ${CMAKE_SOURCE_DIR}/framework/include
    ${CMAKE_SOURCE_DIR}/order/include
    ${CMAKE_SOURCE_DIR}/trade/include
    ${CMAKE_SOURCE_DIR}/risk/include
    ${CMAKE_SOURCE_DIR}/thirdparty/include
)

# Enable testing
enable_testing()
add_test(NAME tradeX_test COMMAND tradeX_test)