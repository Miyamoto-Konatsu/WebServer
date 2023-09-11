#include "db/db.h"
#include "pool/sqlconnpool.h"
#include <cassert>
int main() {
    SqlConnPool::getInstance()->init("root", "root", "webserver", 3306);
    UserModel user("test", "test");
    assert(UserSql::getInstance()->addUser(user));
    assert(UserSql::getInstance()->hasUser(user));
    return 0;
}

