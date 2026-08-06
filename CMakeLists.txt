cmake_minimum_required(VERSION 3.20)
project(voice-center VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(voice-center
  src/main.cpp
  src/core/Logger.cpp
)
target_include_directories(voice-center PRIVATE src)
