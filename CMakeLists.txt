cmake_minimum_required(VERSION 3.0)

project(types-values)

add_executable(playground src/playground.cpp)
target_compile_features(playground PUBLIC cxx_std_17)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
  "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")

add_custom_target(compile_commands ALL)

add_custom_command(
  TARGET compile_commands POST_BUILD
  COMMAND touch ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
  COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR})
