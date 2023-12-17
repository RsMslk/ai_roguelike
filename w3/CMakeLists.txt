cmake_minimum_required(VERSION 3.13)

project(w3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE HW3_SOURCES1 . ./*.[ch]pp)
file(GLOB_RECURSE HW3_SOURCES2 . ./*.[ch])

add_executable(hw3 ${HW3_SOURCES1} ${HW3_SOURCES2})
target_link_libraries(hw3 PUBLIC project_options project_warnings)
target_link_libraries(hw3 PUBLIC raylib flecs)

add_custom_command(TARGET hw3 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:hw3> $<TARGET_FILE_DIR:hw3>
  COMMAND_EXPAND_LISTS
)
add_custom_command(TARGET hw3 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:hw3>/assets
  COMMAND_EXPAND_LISTS
)
