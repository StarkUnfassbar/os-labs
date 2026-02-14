cmake_minimum_required(VERSION 3.28)

project(lab2 CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

add_library(lab_library lab_library.cpp)
target_include_directories(lab_library PUBLIC include)

add_executable(lab main.cpp)

target_link_libraries(lab lab_library)