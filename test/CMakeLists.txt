file(GLOB_RECURSE sources "${CMAKE_SOURCE_DIR}/webserver/*/*.c*") 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
foreach(target logtest heaptimertest dbtest sqlpooltest threadpooltest httpreqtest httprestest)
    add_executable(${target} "${target}.cpp" ${sources})
    target_link_libraries(${target} PRIVATE pthread mysqlclient )
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/webserver )
endforeach()