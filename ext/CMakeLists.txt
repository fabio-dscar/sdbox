cmake_minimum_required (VERSION 3.20)

include(FetchContent)

# ---------------------------------------------------------------------------------------
#     OpenGL
# ---------------------------------------------------------------------------------------
find_package(OpenGL REQUIRED)

set(OPENGL_LIBRARIES ${OPENGL_LIBRARIES} PARENT_SCOPE)

# ---------------------------------------------------------------------------------------
#     glad
# ---------------------------------------------------------------------------------------
set(GLAD_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glad/include PARENT_SCOPE)
set(GLAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c PARENT_SCOPE)

add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c)

target_include_directories(glad PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)

set_property(TARGET glad PROPERTY FOLDER "ext")

# ---------------------------------------------------------------------------------------
#     glfw
# ---------------------------------------------------------------------------------------
set(BUILD_SHARED_LIBS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        d097e35743ee49d38811a89ea29c7f2525dc8d6a
)

FetchContent_MakeAvailable(glfw)

set(GLFW_INCLUDE_DIR ${glfw_SOURCE_DIR}/include PARENT_SCOPE)
set(GLFW_LIBRARIES glfw PARENT_SCOPE)

# ---------------------------------------------------------------------------------------
#     spdlog
# ---------------------------------------------------------------------------------------
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL " " FORCE)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        ac55e60488032b9acde8940a5de099541c4515da
)

FetchContent_MakeAvailable(spdlog)

set(SPDLOG_INCLUDE_DIR ${spdlog_SOURCE_DIR}/include PARENT_SCOPE)
set(SPDLOG_LIBRARIES spdlog PARENT_SCOPE)