#pragma once
#include "pool/sqlconnpool.h"

struct UserModel {
    UserModel() = default;
    UserModel(const std::string username, const std::string passwd = "") :
        username_(username), passwd_(passwd) {
    }
    
    std::string username_;
    std::string passwd_;
};


class UserSql {
public:
    static UserSql *getInstance() {
        static UserSql instance;
        return &instance;
    }

    bool addUser(const UserModel &user);
    bool hasUser(const UserModel &user);
    bool hasUser(const std::string &username);

    UserSql(const UserSql &) = delete;
    UserSql &operator=(const UserSql &) = delete;

private:
    UserSql() = default;
    ~UserSql() = default;
};