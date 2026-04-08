#include "tradex/auth/auth.h"
#include "tradex/framework/log.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace tradex {
namespace auth {

Session::Session(int64_t user_id, const std::string& token)
    : user_id_(user_id), token_(token) {
    expire_time_ = std::time(nullptr) + 3600;  // 1 hour
}

bool Session::isValid() const {
    return std::time(nullptr) < expire_time_;
}

void Session::extend() {
    expire_time_ = std::time(nullptr) + 3600;
}

AuthService::AuthService() : session_timeout_(3600) {}

bool AuthService::initialize() {
    LOG_INFO("AuthService initialized");
    return true;
}

std::string AuthService::generateSalt() {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

    std::string salt;
    for (int i = 0; i < 32; ++i) {
        salt += charset[dis(gen)];
    }
    return salt;
}

std::string AuthService::generateToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::ostringstream oss;
    oss << std::hex;
    for (int i = 0; i < 32; ++i) {
        oss << dis(gen);
    }
    return oss.str();
}

std::string AuthService::hashPassword(const std::string& password, const std::string& salt) {
    // Simplified - use proper hashing in production
    std::string combined = password + salt;
    std::hash<std::string> hasher;
    std::ostringstream oss;
    oss << std::hex << hasher(combined);
    return oss.str();
}

bool AuthService::login(const std::string& username, const std::string& password,
                         int64_t& user_id, std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = users_by_username_.find(username);
    if (it == users_by_username_.end()) {
        return false;
    }

    auto user_it = users_by_id_.find(it->second);
    if (user_it == users_by_id_.end()) {
        return false;
    }

    const auto& user = user_it->second;
    if (!user.enabled) {
        return false;
    }

    std::string hash = hashPassword(password, user.salt);
    if (hash != user.password_hash) {
        return false;
    }

    // Create session
    user_id = user.user_id;
    token = generateToken();
    sessions_by_token_[token] = std::make_shared<Session>(user_id, token);

    LOG_INFO("User logged in: " + username);
    return true;
}

bool AuthService::logout(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_by_token_.erase(token) > 0;
}

bool AuthService::verifyToken(const std::string& token, int64_t& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_by_token_.find(token);
    if (it == sessions_by_token_.end()) {
        return false;
    }

    auto session = it->second;
    if (!session->isValid()) {
        sessions_by_token_.erase(it);
        return false;
    }

    user_id = session->userId();
    session->extend();
    return true;
}

bool AuthService::createUser(const std::string& username, const std::string& password,
                             UserRole role, int64_t account_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (users_by_username_.find(username) != users_by_username_.end()) {
        return false;
    }

    std::string salt = generateSalt();
    std::string hash = hashPassword(password, salt);

    User user;
    user.user_id = users_by_id_.size() + 1;
    user.username = username;
    user.password_hash = hash;
    user.salt = salt;
    user.role = role;
    user.account_id = account_id;
    user.enabled = true;
    user.create_time = std::time(nullptr);
    user.last_login_time = 0;

    users_by_id_[user.user_id] = user;
    users_by_username_[username] = user.user_id;

    return true;
}

bool AuthService::deleteUser(int64_t user_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = users_by_id_.find(user_id);
    if (it == users_by_id_.end()) {
        return false;
    }

    users_by_username_.erase(it->second.username);
    users_by_id_.erase(it);
    return true;
}

bool AuthService::enableUser(int64_t user_id, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = users_by_id_.find(user_id);
    if (it == users_by_id_.end()) {
        return false;
    }

    it->second.enabled = enabled;
    return true;
}

bool AuthService::changePassword(int64_t user_id, const std::string& old_password,
                                  const std::string& new_password) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = users_by_id_.find(user_id);
    if (it == users_by_id_.end()) {
        return false;
    }

    auto& user = it->second;
    std::string old_hash = hashPassword(old_password, user.salt);
    if (old_hash != user.password_hash) {
        return false;
    }

    user.salt = generateSalt();
    user.password_hash = hashPassword(new_password, user.salt);
    return true;
}

bool AuthService::hasPermission(int64_t user_id, UserRole required_role) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = users_by_id_.find(user_id);
    if (it == users_by_id_.end()) {
        return false;
    }

    return static_cast<int>(it->second.role) <= static_cast<int>(required_role);
}

void AuthService::cleanupExpiredSessions() {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_by_token_.begin();
    while (it != sessions_by_token_.end()) {
        if (!it->second->isValid()) {
            it = sessions_by_token_.erase(it);
        } else {
            ++it;
        }
    }
}

// PermissionChecker implementation
bool PermissionChecker::canTrade(UserRole role) {
    return role == UserRole::Admin || role == UserRole::Trader || role == UserRole::Algo;
}

bool PermissionChecker::canView(UserRole role) {
    return true;
}

bool PermissionChecker::canAdmin(UserRole role) {
    return role == UserRole::Admin;
}

bool PermissionChecker::canAlgo(UserRole role) {
    return role == UserRole::Algo || role == UserRole::Admin;
}

} // namespace auth
} // namespace tradex