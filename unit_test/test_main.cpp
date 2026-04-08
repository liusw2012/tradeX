// Simple test runner
#include <iostream>
#include "test_framework.h"

// Test counter
int passed = 0;
int failed = 0;

// Test function declarations
void test_order_creation();
void test_order_status();
void test_config_loading();

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "         tradeX Unit Tests             " << std::endl;
    std::cout << "========================================" << std::endl;

    // Run order tests
    std::cout << "\nOrder Tests:" << std::endl;
    RUN_TEST(order_creation);
    RUN_TEST(order_status);

    // Run config tests
    std::cout << "\nConfig Tests:" << std::endl;
    RUN_TEST(config_loading);

    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return failed > 0 ? 1 : 0;
}