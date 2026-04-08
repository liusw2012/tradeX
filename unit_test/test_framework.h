// Simple test framework header
#pragma once

#include <iostream>
#include <string>
#include <stdexcept>

// Test macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "  " << #name << "... "; \
    try { \
        test_##name(); \
        std::cout << "PASSED" << std::endl; \
        passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        failed++; \
    } catch (...) { \
        std::cout << "FAILED: unknown exception" << std::endl; \
        failed++; \
    } \
} while(0)

#define ASSERT_TRUE(cond) if (!(cond)) throw std::runtime_error("Assertion failed: " #cond)
#define ASSERT_EQ(a, b) if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " == " #b)
#define ASSERT_NE(a, b) if ((a) == (b)) throw std::runtime_error("Assertion failed: " #a " != " #b)

// Test counter (defined in test_main.cpp)
extern int passed;
extern int failed;

// Test function declarations
void test_order_creation();
void test_order_status();
void test_config_loading();