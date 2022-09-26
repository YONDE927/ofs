cmake_minimum_required(VERSION 3.18.0)

project(file-diff C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb")

set(srcs "connection.cc" "ftpserver.cc" "ftpclient.cc" "common.cc") 
set(test-srcs "test-syncfile.cc")

add_library(syncfile STATIC ${srcs})

add_executable(repl "ftprepl.cc")
target_link_libraries(repl syncfile)

add_executable(repl2 "ftprepl2.cc")
target_link_libraries(repl2 syncfile)

add_executable(test-syncfile ${test-srcs})
target_link_libraries(test-syncfile syncfile)
