cmake_minimum_required(VERSION 3.10)

# MSYS2/MSVC workaround: Skip compiler test BEFORE project() is called
# This prevents CMake from testing the compiler during configuration
set(CMAKE_CXX_COMPILER_WORKS TRUE)
set(CMAKE_C_COMPILER_WORKS TRUE)

project(tradeX CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# MSYS2 workaround: Use -pipe for faster compilation
if(MINGW OR MSYS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe")

    # Create temp dir in accessible location
    #file(MAKE_DIRECTORY "D:/tmp")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/temp)
endif()

# Set environment variables for temp files (works for compiler and runtime)
# set(ENV{TMP} "D:/tmp")
# set(ENV{TEMP} "D:/tmp")
set(ENV{TMP}  ${CMAKE_BINARY_DIR}/temp)
set(ENV{TEMP} ${CMAKE_BINARY_DIR}/temp)

# Warning options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(WARNING_FLAGS -Wall -Wextra -Wpedantic)
elseif(MSVC)
    set(WARNING_FLAGS /W4)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform detection with cache variable for override
# Try to auto-detect, but allow manual override with -DWINDOWS=ON or -DWINDOWS=OFF
if(NOT DEFINED WINDOWS OR WINDOWS STREQUAL "")
    # Check multiple indicators for Windows/MinGW/MSYS2
    set(WINDOWS FALSE)
    if(WIN32 OR MINGW OR MSYS OR CYGWIN OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(WINDOWS TRUE)
    endif()
    # Also check if using MinGW compiler on Windows
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # MinGW on Windows or MSYS2
        string(TOLOWER "${CMAKE_CXX_COMPILER}" COMPILER_LOWER)
        if(COMPILER_LOWER MATCHES "mingw" OR COMPILER_LOWER MATCHES "g\\+\\+")
            set(WINDOWS TRUE)
        endif()
    endif()
    # Check MSYS environment
    if(DEFINED ENV{MSYSTEM_PREFIX} OR DEFINED ENV{MSYS})
        set(WINDOWS TRUE)
    endif()
endif()

# Make WINDOWS a cache variable so it propagates to subdirectories
set(WINDOWS ${WINDOWS} CACHE BOOL "Build for Windows (MinGW/MSYS2/MSVC)" FORCE)

message(STATUS "=== Platform Detection ===")
message(STATUS "WINDOWS: ${WINDOWS}")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "==========================")

# Find MySQL
find_package(MySQL QUIET)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/framework/include
    ${CMAKE_SOURCE_DIR}/dao/include
    ${CMAKE_SOURCE_DIR}/order/include
    ${CMAKE_SOURCE_DIR}/trade/include
    ${CMAKE_SOURCE_DIR}/risk/include
    ${CMAKE_SOURCE_DIR}/task/include
    ${CMAKE_SOURCE_DIR}/auth/include
    ${CMAKE_SOURCE_DIR}/gateway/include
    ${CMAKE_SOURCE_DIR}/market/include
    ${CMAKE_SOURCE_DIR}/algo/include
    ${CMAKE_SOURCE_DIR}/strategy/include
    ${CMAKE_SOURCE_DIR}/admin/include
    ${CMAKE_SOURCE_DIR}/net/include
    ${CMAKE_SOURCE_DIR}/thirdparty/include
)

# Third-party libraries
add_subdirectory(thirdparty)

# Create logs directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/logs)

# Framework
add_subdirectory(framework)

# DAO layer
add_subdirectory(dao)

# Core modules
add_subdirectory(order)
add_subdirectory(trade)
add_subdirectory(risk)
add_subdirectory(task)
add_subdirectory(auth)
add_subdirectory(gateway)
add_subdirectory(market)
add_subdirectory(algo)
add_subdirectory(strategy)
add_subdirectory(admin)
add_subdirectory(net)

# Main executable (combines all modules for monolithic build)
add_executable(tradeX
    src/main.cpp
)

# Platform-specific libraries
# Windows (including MinGW/MSYS2) needs ws2_32 for sockets
# Linux/macOS needs pthread
if(WINDOWS)
    set(PLATFORM_LIBS ws2_32)
else()
    set(PLATFORM_LIBS pthread)
endif()

target_link_libraries(tradeX
    tradex_framework
    tradex_dao
    tradex_order
    tradex_trade
    tradex_risk
    tradex_task
    tradex_auth
    tradex_gateway
    tradex_market
    tradex_algo
    tradex_strategy
    tradex_admin
    tradex_net
    ${PLATFORM_LIBS}
)

target_include_directories(tradeX PRIVATE ${CMAKE_SOURCE_DIR})

# Unit tests
enable_testing()
add_subdirectory(unit_test)

# CLI Client
add_subdirectory(cli)

# Installation
install(TARGETS tradeX
    RUNTIME DESTINATION bin
)
