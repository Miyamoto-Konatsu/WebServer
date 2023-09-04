
file(GLOB_RECURSE sources "${CMAKE_SOURCE_DIR}/webserver/*/*.c*") 

foreach(target logtest heaptimertest threadpooltest httpreqtest)
    add_executable(${target} "${target}.cpp" ${sources})
    target_link_libraries(${target} PRIVATE pthread)
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR}/webserver )
endforeach()