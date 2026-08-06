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

option(VOICE_CENTER_DRY_ONLY "Build without SDL2 / SdlPlayer (dry-run player only)" OFF)

set(VOICE_CORE_SOURCES
  src/core/Logger.cpp
  src/core/Config.cpp
  src/core/StateTracker.cpp
  src/core/EventQueue.cpp
  src/core/UdpListener.cpp
  src/core/DryRunPlayer.cpp
  src/core/VoiceManager.cpp
)

if(NOT VOICE_CENTER_DRY_ONLY)
  find_package(SDL2 REQUIRED)
  list(APPEND VOICE_CORE_SOURCES src/platform/windows/SdlPlayer.cpp)
endif()

add_library(voice_core ${VOICE_CORE_SOURCES})
target_include_directories(voice_core PUBLIC src)
target_link_libraries(voice_core PUBLIC nlohmann_json::nlohmann_json)
if(WIN32)
  target_link_libraries(voice_core PUBLIC ws2_32)
endif()

if(VOICE_CENTER_DRY_ONLY)
  target_compile_definitions(voice_core PUBLIC VOICE_CENTER_DRY_ONLY)
endif()

if(NOT VOICE_CENTER_DRY_ONLY)
  # Console app uses normal main(); avoid SDL_main rewrite.
  target_compile_definitions(voice_core PUBLIC SDL_MAIN_HANDLED)
  if(TARGET SDL2::SDL2)
    target_link_libraries(voice_core PUBLIC SDL2::SDL2)
  else()
    target_include_directories(voice_core PUBLIC ${SDL2_INCLUDE_DIRS})
    target_link_libraries(voice_core PUBLIC ${SDL2_LIBRARIES})
  endif()
endif()

add_executable(voice-center src/main.cpp)
target_link_libraries(voice-center PRIVATE voice_core)

add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests PRIVATE voice_core)

if(NOT VOICE_CENTER_DRY_ONLY AND WIN32)
  # Copy SDL2.dll next to executables for Release/Debug runs.
  if(DEFINED SDL2_BINDIR)
    set(_sdl2_dll "${SDL2_BINDIR}/SDL2.dll")
  elseif(DEFINED SDL2_LIBDIR)
    set(_sdl2_dll "${SDL2_LIBDIR}/SDL2.dll")
  endif()
  if(EXISTS "${_sdl2_dll}")
    add_custom_command(TARGET voice-center POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_sdl2_dll}"
        $<TARGET_FILE_DIR:voice-center>
      COMMENT "Copy SDL2.dll to voice-center output dir")
  endif()
endif()
