# Network module
add_library(tradex_net STATIC
    net_server.cpp
    net_client.cpp
    protocol.cpp
)

# Platform-specific libraries for sockets
# Use the WINDOWS variable from parent CMakeLists.txt
if(WINDOWS)
    set(NET_LIBS ws2_32)
else()
    set(NET_LIBS pthread)
endif()

target_link_libraries(tradex_net
    tradex_framework
    tradex_order
    tradex_trade
    tradex_risk
    ${NET_LIBS}
)

target_include_directories(tradex_net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)