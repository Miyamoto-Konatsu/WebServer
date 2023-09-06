#pragma once

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>

class SqlConnPool {
public:
    void init(const std::string &user, const std::string &passwd,
              const std::string &db, unsigned short port,
              int maxConnCount = 16);

    static SqlConnPool *getInstance();

    std::shared_ptr<MYSQL> getConn();

    SqlConnPool(const SqlConnPool &) = delete;
    SqlConnPool &operator=(const SqlConnPool &) = delete;

private:
    SqlConnPool();
    ~SqlConnPool();

    std::string user_;
    std::string passwd_;
    std::string db_;
    unsigned short port_;

    int maxConnCount_;

    std::queue<MYSQL *> connQueue_;
    std::mutex mtx_;
    std::condition_variable cv_;
};