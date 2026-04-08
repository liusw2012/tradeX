#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace tradex {
namespace auth {

// User role
enum class UserRole {
    Admin = 1,
    Trader = 2,
    Viewer = 3,
    Algo = 4
};

// User account
struct User {
    int64_t user_id;
    std::string username;
    std::string password_hash;
    std::string salt;
    UserRole role;
    int64_t account_id;
    bool enabled;
    int64_t create_time;
    int64_t last_login_time;
};

// Session
class Session {
public:
    Session(int64_t user_id, const std::string& token);

    int64_t userId() const { return user_id_; }
    const std::string& token() const { return token_; }
    int64_t expireTime() const { return expire_time_; }

    bool isValid() const;
    void extend();

private:
    int64_t user_id_;
    std::string token_;
    int64_t expire_time_;
};

// Authentication service
class AuthService {
public:
    AuthService();
    ~AuthService() = default;

    bool initialize();

    // Authentication
    bool login(const std::string& username, const std::string& password,
               int64_t& user_id, std::string& token);

    bool logout(const std::string& token);

    bool verifyToken(const std::string& token, int64_t& user_id);

    // User management
    bool createUser(const std::string& username, const std::string& password,
                    UserRole role, int64_t account_id);

    bool deleteUser(int64_t user_id);

    bool enableUser(int64_t user_id, bool enabled);

    bool changePassword(int64_t user_id, const std::string& old_password,
                        const std::string& new_password);

    // Permission check
    bool hasPermission(int64_t user_id, UserRole required_role);

    // Session management
    void cleanupExpiredSessions();

private:
    std::string hashPassword(const std::string& password, const std::string& salt);
    std::string generateToken();
    std::string generateSalt();

    mutable std::mutex mutex_;

    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_by_token_;
    std::unordered_map<int64_t, User> users_by_id_;
    std::unordered_map<std::string, int64_t> users_by_username_;

    int64_t session_timeout_;  // seconds
};

// Permission checker
class PermissionChecker {
public:
    static bool canTrade(UserRole role);
    static bool canView(UserRole role);
    static bool canAdmin(UserRole role);
    static bool canAlgo(UserRole role);
};

} // namespace auth
} // namespace tradex