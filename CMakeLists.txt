cmake_minimum_required(VERSION 3.21)

project(MainLevelsListChange)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include($ENV{GEODE_SDK}/cmake/Geode.cmake)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Geode::Geode)

setup_geode_mod(${PROJECT_NAME})
