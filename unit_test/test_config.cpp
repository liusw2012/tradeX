// Test config functionality
#include "tradex/framework/config.h"
#include "test_framework.h"

// Test config loading
void test_config_loading() {
    using namespace tradex::framework;

    // Test default config
    Config& config = Config::instance();

    // Test getting non-existent key with default
    ConfigValue defaultVal = config.get("nonexistent.key", ConfigValue("default"));
    ASSERT_EQ(defaultVal.asString(), "default");

    // Test config value types
    ConfigValue intVal("123");
    ASSERT_EQ(intVal.asInt(), 123);

    ConfigValue doubleVal("10.5");
    ASSERT_EQ(doubleVal.asDouble(), 10.5);

    ConfigValue boolVal("true");
    ASSERT_EQ(boolVal.asBool(), true);
}