#include "sqlconnpool.h"
#include <cstdio>
#include <memory>
#include <mutex>
#include <mysql/mysql.h>

void SqlConnPool::init(const std::string &user, const std::string &passwd,
                       const std::string &db, unsigned short port,
                       int maxConnCount) {
    user_ = user;
    passwd_ = passwd;
    db_ = db;
    port_ = port;
    maxConnCount_ = maxConnCount;

    for (int i = 0; i < maxConnCount_; ++i) {
        MYSQL *conn = mysql_init(nullptr);
        if (conn == nullptr) { exit(1); }
        if (mysql_real_connect(conn, "0.0.0.0", user_.c_str(), passwd_.c_str(),
                               db_.c_str(), port_, nullptr, 0)
            == nullptr) {
            printf("%s %d: mysql_real_connect error", __FILE__, __LINE__);
            exit(1);
        }
        connQueue_.push(conn);
    }
}

SqlConnPool *SqlConnPool::getInstance() {
    static SqlConnPool instance;
    return &instance;
}

SqlConnPool::SqlConnPool() {
}

SqlConnPool::~SqlConnPool() {
    std::lock_guard<std::mutex> lock(mtx_);
    for (int i = 0; i < maxConnCount_; ++i) {
        MYSQL *conn = connQueue_.front();
        connQueue_.pop();
        mysql_close(conn);
    }
}

std::shared_ptr<MYSQL> SqlConnPool::getConn() {
    std::unique_lock<std::mutex> lock(mtx_);
    while (connQueue_.empty()) { cv_.wait(lock); }
    MYSQL *conn = connQueue_.front();
    connQueue_.pop();
    return std::shared_ptr<MYSQL>(conn, [this](MYSQL *conn) {
        std::unique_lock<std::mutex> lock(mtx_);
        connQueue_.push(conn);
        cv_.notify_one();
    });
}