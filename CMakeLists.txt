cmake_minimum_required(VERSION 3.0.0)
project(TinyWebServer VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

file(GLOB_RECURSE sources "${CMAKE_SOURCE_DIR}/webserver/*/*.c*") 
add_executable(server ./main.cpp ${sources})
target_link_libraries(server PRIVATE pthread)
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/webserver)

add_subdirectory(./test)
