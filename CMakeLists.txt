cmake_minimum_required(VERSION 3.16)

project(Escape_Velocity LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Escape_Velocity main.cpp)
target_link_libraries(Escape_Velocity pthread tbb)