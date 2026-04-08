#pragma once

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>

namespace tradex {
namespace framework {

// Configuration value wrapper
class ConfigValue {
public:
    ConfigValue() : value_(), is_set_(false) {}
    explicit ConfigValue(const std::string& value) : value_(value), is_set_(true) {}

    bool isSet() const { return is_set_; }

    std::string asString() const { return value_; }

    int asInt() const { return std::stoi(value_); }
    long asLong() const { return std::stol(value_); }
    double asDouble() const { return std::stod(value_); }
    bool asBool() const {
        return value_ == "true" || value_ == "1" || value_ == "yes";
    }

    template<typename T>
    T as() const;

private:
    std::string value_;
    bool is_set_;
};

template<>
inline std::string ConfigValue::as<std::string>() const { return asString(); }

template<>
inline int ConfigValue::as<int>() const { return asInt(); }

template<>
inline long ConfigValue::as<long>() const { return asLong(); }

template<>
inline double ConfigValue::as<double>() const { return asDouble(); }

template<>
inline bool ConfigValue::as<bool>() const { return asBool(); }

// Configuration manager
class Config {
public:
    static Config& instance();

    // Load from file
    bool loadFromFile(const std::string& filepath);

    // Load from JSON string
    bool loadFromJson(const std::string& json);

    // Get configuration value
    ConfigValue get(const std::string& key) const;
    ConfigValue get(const std::string& key, const ConfigValue& default_value) const;

    // Set configuration value (runtime)
    void set(const std::string& key, const ConfigValue& value);

    // Check if key exists
    bool has(const std::string& key) const;

    // Get all keys with prefix
    std::map<std::string, ConfigValue> getByPrefix(const std::string& prefix) const;

    // Reload configuration
    bool reload();

private:
    Config();
    ~Config() = default;

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::string filepath_;
    std::map<std::string, ConfigValue> values_;
    mutable std::mutex mutex_;

    void parseJson(const std::string& json);
    std::string trim(const std::string& str);
};

// Convenience accessor
#define CONFIG(key) tradex::framework::Config::instance().get(key)
#define CONFIG_STRING(key) CONFIG(key).asString()
#define CONFIG_INT(key) CONFIG(key).asInt()
#define CONFIG_DOUBLE(key) CONFIG(key).asDouble()
#define CONFIG_BOOL(key) CONFIG(key).asBool()

} // namespace framework
} // namespace tradex