#include "db.h"
#include "log/log.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <cstring>
#include <openssl/types.h>

static std::string encryptPasswd(const std::string &passwd) {
        // 创建一个哈希上下文
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();

    // 指定要使用的哈希算法，例如SHA-256
    const EVP_MD *md = EVP_sha256();

    // 初始化哈希上下文
    EVP_DigestInit_ex(mdctx, md, NULL);

    // 添加密码数据到哈希上下文
    EVP_DigestUpdate(mdctx, passwd.c_str(), passwd.size());

    // 获取哈希结果
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    unsigned char md_value_str[EVP_MAX_MD_SIZE];

    std::string hashResult;
    for (unsigned int i = 0; i < md_len; i++) {
        char hex[3] = {0};
        sprintf(hex, "%02x", md_value[i]);
        hashResult += hex;
    }

    // 清理哈希上下文
    EVP_MD_CTX_free(mdctx);
    return hashResult;
}

bool UserSql::addUser(const UserModel &user) {
    std::shared_ptr<MYSQL> conn = SqlConnPool::getInstance()->getConn();
    if (conn == nullptr) {
        return false;
    }
    if(user.username_ == "" || user.passwd_ == "") {
        LOG_INFO("Add user failed! username or passwd is empty!");
        return false;
    }
    std::string sql = "INSERT INTO user(username, passwd) VALUES('"
        + user.username_ + "', '" + encryptPasswd(user.passwd_) + "')";
    int ret = mysql_query(conn.get(), sql.c_str());
    if (ret != 0) {
        LOG_ERROR("Insert into user failed! sql: %s, error: %s",
                  sql.c_str(), mysql_error(conn.get()));
        return false;
    }
    return true;
}

bool UserSql::hasUser(const UserModel &user) {
    std::shared_ptr<MYSQL> conn = SqlConnPool::getInstance()->getConn();
    if (conn == nullptr) {
        return false;
    }
    if(user.username_ == "" || user.passwd_ == "") {
        LOG_INFO("Has user failed! username or passwd is empty!");
        return false;
    }
    std::string sql = "SELECT * FROM user WHERE username='" + user.username_
        + "' AND passwd='" + encryptPasswd(user.passwd_) + "'";
    int ret = mysql_query(conn.get(), sql.c_str());
    if (ret != 0) {
        LOG_ERROR("Select from user failed! sql: %s, error: %s",
                  sql.c_str(), mysql_error(conn.get()));
        return false;
    }
    MYSQL_RES *res = mysql_store_result(conn.get());
    if (res == nullptr) {
        LOG_ERROR("Store result failed! sql: %s, error: %s",
                  sql.c_str(), mysql_error(conn.get()));
        return false;
    }
    int num = mysql_num_rows(res);
    mysql_free_result(res);
    if (num > 0) {
        return true;
    }
    return false;
}

bool UserSql::hasUser(const std::string &username) {
    std::shared_ptr<MYSQL> conn = SqlConnPool::getInstance()->getConn();
    if (conn == nullptr) {
        return false;
    }
    if(username == "") {
        LOG_INFO("Has user failed! username is empty!");
        return false;
    }
    std::string sql = "SELECT * FROM user WHERE username='" + username + "'";
    int ret = mysql_query(conn.get(), sql.c_str());
    if (ret != 0) {
        LOG_ERROR("Select from user failed! sql: %s, error: %s",
                  sql.c_str(), mysql_error(conn.get()));
        return false;
    }
    MYSQL_RES *res = mysql_store_result(conn.get());
    if (res == nullptr) {
        LOG_ERROR("Store result failed! sql: %s, error: %s",
                  sql.c_str(), mysql_error(conn.get()));
        return false;
    }
    int num = mysql_num_rows(res);
    mysql_free_result(res);
    if (num > 0) {
        return true;
    }
    return false;
}

