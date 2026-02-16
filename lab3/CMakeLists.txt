cmake_minimum_required(VERSION 3.28)

project(lab3 CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

if (UNIX)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

add_executable(lab main.cpp)

target_include_directories(lab PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (UNIX)
    target_link_libraries(lab Threads::Threads)
endif()