#pragma once

#include <string>

namespace tradex {
namespace framework {

// Build information
constexpr const char* kVersion = "1.0.0";
constexpr const char* kBuildDate = __DATE__;
constexpr const char* kBuildTime = __TIME__;
constexpr const char* kGitCommit = "initial";

inline std::string getVersion() { return kVersion; }
inline std::string getBuildInfo() {
    return std::string("tradeX ") + kVersion +
           " built on " + kBuildDate + " " + kBuildTime;
}

} // namespace framework
} // namespace tradex