#include "http/httpresponse.h"
#include <iostream>

int main() {
    auto staticPath = "/root/code/cpp/TinyWebServer/static/";
    HttpResponse response(staticPath);
    Buffer buffer;
    response.reset(200, false, "/ind11ex.html");
    response.makeResponse(buffer);
    std::cout << buffer.retrieveAllToStr() << std::endl;
    return 0;
}