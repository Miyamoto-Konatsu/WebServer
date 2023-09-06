#include "pool/sqlconnpool.h"
#include <cstdio>
#include <mysql/mysql.h>
#include <string>
#include <thread>
#include <vector>

void func(int threadid) {
    for (int i = 0; i < 100; ++i) {
        auto sqlconn = SqlConnPool::getInstance()->getConn();
        char sql[1024] = {0};

        sprintf(sql, "insert into user values('%s','%s')", "test",
                (std::to_string(threadid) + std::to_string(i)).c_str());
        if (mysql_query(sqlconn.get(), sql) == 0) {
            // 查询成功执行
        } else {
            // 查询执行失败
            fprintf(stderr, "Query execution failed: %s\n",
                    mysql_error(sqlconn.get()));
        }

        // sprintf(sql, "select * from user where username=%s and
        // password=%s",
        //         "test", "test");
        // mysql_query(sqlconn.get(), sql);
        // auto res = mysql_store_result(sqlconn.get());
        // if (res == nullptr) {
        //     fprintf(stderr, "Query execution failed: %s\n",
        //             mysql_error(sqlconn.get()));
        //     return 0;
        // }
        // while (auto row = mysql_fetch_row(res)) {
        //     printf("%s\n", row[0]);
        //     printf("%s\n", row[1]);
        // }

        // mysql_free_result(res);
    }
}
int main() {
    SqlConnPool::getInstance()->init("root", "root", "webserver", 3306);
    std::vector<std::thread> threads;
    for (int i = 0; i < 16; ++i) { threads.push_back(std::thread(&func, i)); }
    for (auto &t : threads) { t.join(); }
    return 0;
}