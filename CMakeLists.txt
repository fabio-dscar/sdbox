cmake_minimum_required(VERSION 3.20)

project(sdbox VERSION 0.1 LANGUAGES CXX C)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

##################################
## Third party libs
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ext)

set(SDBOX_SOURCES
  src/app.cpp
  src/win.cpp
  src/util/util.cpp
  src/util/log.cpp
  src/util/thread.cpp
  src/graphics/shader.cpp
  src/graphics/graphics.cpp
  src/graphics/buffer.cpp
  src/graphics/ringbuffer.cpp
  src/watcher/watcher.cpp
  src/watcher/inotifywatcher.cpp
  ${GLAD_SOURCES}
)

add_executable(sdbox src/main.cpp ${SDBOX_SOURCES})
target_compile_features(sdbox PUBLIC cxx_std_20)
target_compile_definitions(sdbox PUBLIC "$<$<CONFIG:Debug>:DEBUG>")
target_include_directories(sdbox PUBLIC
  src
  src/watcher
  src/util
  src/graphics
  ext  
  ${GLAD_INCLUDE_DIR}
  ${SPDLOG_INCLUDE_DIR}
  ${GLM_INCLUDE_DIR}
)
target_link_libraries(sdbox PRIVATE
  glad
  ${OPENGL_LIBRARIES}
  ${GLFW_LIBRARIES}
  ${SPDLOG_LIBRARIES}
  ${BACKWARD_LIBRARIES}
)

add_custom_command(TARGET sdbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/glsl ${CMAKE_BINARY_DIR}/glsl
)

set(DEBUG_FLAGS -Wall -Wextra -Wpedantic)
set(RELEASE_FLAGS -O3 -march=native)
target_compile_options(sdbox PRIVATE "$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:${RELEASE_FLAGS}>" 
                                     "$<$<CONFIG:Debug>:${DEBUG_FLAGS}>")