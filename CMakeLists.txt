cmake_minimum_required(VERSION 3.20)

project(MyThreadPool VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(MyThreadPool src/main.cpp)
target_include_directories(MyThreadPool PRIVATE include)