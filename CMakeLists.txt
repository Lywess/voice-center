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

set(_sdl2_dll "")
if(NOT VOICE_CENTER_DRY_ONLY)
  # 官方 VC 开发包的 sdl2-config 可能标成 32+64bit，被新版 CMake 拒收；先试 CONFIG，失败再手动找。
  find_package(SDL2 QUIET CONFIG)
  if(NOT TARGET SDL2::SDL2 AND NOT SDL2_FOUND)
    find_path(SDL2_INCLUDE_DIR SDL.h
      PATH_SUFFIXES include SDL2
      PATHS ENV SDL2DIR ${CMAKE_PREFIX_PATH}
    )
    find_library(SDL2_LIBRARY
      NAMES SDL2
      PATH_SUFFIXES lib/x64 lib
      PATHS ENV SDL2DIR ${CMAKE_PREFIX_PATH}
    )
    if(NOT SDL2_INCLUDE_DIR OR NOT SDL2_LIBRARY)
      message(FATAL_ERROR "SDL2 not found. Set CMAKE_PREFIX_PATH to SDL2-devel, or -DVOICE_CENTER_DRY_ONLY=ON")
    endif()
    add_library(SDL2::SDL2 UNKNOWN IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
      IMPORTED_LOCATION "${SDL2_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
    )
    get_filename_component(_sdl2_libdir "${SDL2_LIBRARY}" DIRECTORY)
    if(EXISTS "${_sdl2_libdir}/../lib/x64/SDL2.dll")
      set(_sdl2_dll "${_sdl2_libdir}/../lib/x64/SDL2.dll")
    elseif(EXISTS "${_sdl2_libdir}/SDL2.dll")
      set(_sdl2_dll "${_sdl2_libdir}/SDL2.dll")
    elseif(EXISTS "${_sdl2_libdir}/../bin/SDL2.dll")
      set(_sdl2_dll "${_sdl2_libdir}/../bin/SDL2.dll")
    endif()
    message(STATUS "Using manual SDL2: ${SDL2_LIBRARY}")
  endif()
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

# 打包布局：exe 同级带上 config/ 与 assets/（双击或分发都可用）
add_custom_command(TARGET voice-center POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory
    "$<TARGET_FILE_DIR:voice-center>/config"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/config/voice.json"
    "$<TARGET_FILE_DIR:voice-center>/config/voice.json"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:voice-center>/assets"
  COMMENT "Copy config/ and assets/ next to voice-center.exe")

add_executable(core_tests tests/core_tests.cpp)
target_link_libraries(core_tests PRIVATE voice_core)

if(NOT VOICE_CENTER_DRY_ONLY AND WIN32)
  if(NOT _sdl2_dll)
    if(DEFINED SDL2_BINDIR)
      set(_sdl2_dll "${SDL2_BINDIR}/SDL2.dll")
    elseif(DEFINED SDL2_LIBDIR)
      set(_sdl2_dll "${SDL2_LIBDIR}/SDL2.dll")
    endif()
  endif()
  if(_sdl2_dll AND EXISTS "${_sdl2_dll}")
    add_custom_command(TARGET voice-center POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_sdl2_dll}"
        $<TARGET_FILE_DIR:voice-center>
      COMMENT "Copy SDL2.dll to voice-center output dir")
  endif()
endif()
