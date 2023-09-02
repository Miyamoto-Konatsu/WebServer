cmake_minimum_required(VERSION 3.0.0)
project(TinyWebServer VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(server ./main.cpp)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/webserver)

add_subdirectory(./test)
