project(server)
cmake_minimum_required(VERSION 3.19)
add_executable(server server.cpp)
target_link_libraries(server ${Boost_LIBRARIES})
include_directories(Headers)

