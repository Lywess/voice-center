cmake_minimum_required(VERSION 3.8)
project(nlohmann_json VERSION 3.11.3 LANGUAGES CXX)

add_library(nlohmann_json INTERFACE)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
target_include_directories(
  nlohmann_json
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/single_include>
  $<INSTALL_INTERFACE:include>
)
