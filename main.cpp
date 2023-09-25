#include "pool/sqlconnpool.h"
#include "server/webserver.h"
#include <csignal>

void sigHandler(int sig) {
    switch (sig) {
    case SIGPIPE:
    case SIGALRM:
    case SIGINT: WebServer::isClosed_ = 1;
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGPIPE, sigHandler);
    Log::getInstance()->init(1);
    SqlConnPool::getInstance()->init("root","root", "webserver", 3306);
    WebServer server("0.0.0.0", 8081, 4, 10000,
                     "/root/code/cpp/TinyWebServer/static/");
    server.start();

    return 0;
}