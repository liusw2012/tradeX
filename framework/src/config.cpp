#include "tradex/framework/config.h"

namespace tradex {
namespace framework {

Config& Config::instance() {
    static Config config;
    return config;
}

Config::Config() {}

bool Config::loadFromFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    filepath_ = filepath;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    parseJson(buffer.str());
    return true;
}

bool Config::loadFromJson(const std::string& json) {
    std::lock_guard<std::mutex> lock(mutex_);
    parseJson(json);
    return true;
}

void Config::parseJson(const std::string& json) {
    values_.clear();

    std::string key;
    std::string value;
    bool in_key = true;
    bool in_string = false;
    int brace_level = 0;

    for (size_t i = 0; i < json.size(); ++i) {
        char c = json[i];

        if (c == '"' && (i == 0 || json[i-1] != '\\')) {
            in_string = !in_string;
            continue;
        }

        if (in_string) {
            if (in_key) {
                key += c;
            } else {
                value += c;
            }
            continue;
        }

        if (c == '{') {
            brace_level++;
            continue;
        }
        if (c == '}') {
            brace_level--;
            continue;
        }

        if (c == ':' && brace_level == 1) {
            in_key = false;
            continue;
        }

        if (c == ',' && brace_level == 1) {
            if (!key.empty() && !value.empty()) {
                key = trim(key);
                value = trim(value);
                if (!key.empty() && !value.empty()) {
                    values_[key] = ConfigValue(value);
                }
            }
            key.clear();
            value.clear();
            in_key = true;
            continue;
        }

        if (in_key) {
            key += c;
        } else {
            value += c;
        }
    }

    // Handle last key-value pair
    if (!key.empty() && !value.empty()) {
        key = trim(key);
        value = trim(value);
        if (!key.empty() && !value.empty()) {
            values_[key] = ConfigValue(value);
        }
    }
}

std::string Config::trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && (str[start] == ' ' || str[start] == '\t' ||
                                   str[start] == '\n' || str[start] == '\r')) {
        start++;
    }

    size_t end = str.size();
    while (end > start && (str[end-1] == ' ' || str[end-1] == '\t' ||
                          str[end-1] == '\n' || str[end-1] == '\r')) {
        end--;
    }

    return str.substr(start, end - start);
}

ConfigValue Config::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }
    return ConfigValue();
}

ConfigValue Config::get(const std::string& key, const ConfigValue& default_value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = values_.find(key);
    if (it != values_.end()) {
        return it->second;
    }
    return default_value;
}

void Config::set(const std::string& key, const ConfigValue& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_[key] = value;
}

bool Config::has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return values_.find(key) != values_.end();
}

std::map<std::string, ConfigValue> Config::getByPrefix(const std::string& prefix) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, ConfigValue> result;

    for (const auto& kv : values_) {
        if (kv.first.substr(0, prefix.size()) == prefix) {
            result[kv.first] = kv.second;
        }
    }

    return result;
}

bool Config::reload() {
    if (filepath_.empty()) {
        return false;
    }
    return loadFromFile(filepath_);
}

} // namespace framework
} // namespace tradex