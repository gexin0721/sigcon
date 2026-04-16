cmake_minimum_required(VERSION 3.24)

project(sigcon VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIGCON_BUILD_DESKTOP "Build the Qt desktop application" OFF)

if(SIGCON_BUILD_DESKTOP)
    add_subdirectory(desktop)
endif()

message(STATUS "sigcon root scaffold loaded")
