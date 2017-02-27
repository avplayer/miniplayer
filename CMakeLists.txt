cmake_minimum_required(VERSION 3.7.2)

project(miniplayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(Threads REQUIRED)
find_package(FFMPEG REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Quick CONFIG)

add_subdirectory(src)
