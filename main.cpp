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
    signal(SIGPIPE, SIG_IGN); Log::getInstance()->init(0);
    signal(SIGPIPE, sigHandler);
    WebServer server("0.0.0.0", 8081, 4, -1,
                     "/root/code/cpp/TinyWebServer/static/");
    server.start();

    return 0;
}