project(client)
cmake_minimum_required(VERSION 3.19)
add_executable(client client.cpp)
target_link_libraries(client ${Boost_LIBRARIES})
include_directories(Headers)

