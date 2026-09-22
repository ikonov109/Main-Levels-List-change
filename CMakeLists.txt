cmake_minimum_required(VERSION 3.21)

project(MainLevelsListChange)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT GEODE_SDK)
    set(GEODE_SDK "$ENV{GEODE_SDK}")
endif()

include(${GEODE_SDK}/cmake/Geode.cmake)

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Geode::Geode)

setup_geode_mod(${PROJECT_NAME})
