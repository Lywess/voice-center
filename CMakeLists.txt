cmake_minimum_required(VERSION 3.20)
project(voice-center VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/utf-8)
endif()

include(FetchContent)
set(FETCHCONTENT_SOURCE_DIR_JSON "${CMAKE_SOURCE_DIR}/third_party/json")
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

add_library(voice_core
  src/core/Logger.cpp
  src/core/Config.cpp
  src/core/StateTracker.cpp
  src/core/EventQueue.cpp
  src/core/UdpListener.cpp
)
target_include_directories(voice_core PUBLIC src)
target_link_libraries(voice_core PUBLIC nlohmann_json::nlohmann_json)
if(WIN32)
  target_link_libraries(voice_core PUBLIC ws2_32)
endif()

add_executable(voice-center src/main.cpp)
target_link_libraries(voice-center PRIVATE voice_core)

add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests PRIVATE voice_core)
