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

target_compile_options(spdlog PRIVATE "-DSPDLOG_LEVEL_NAMES={ \"TRACE\", \"DEBUG\", \"INFO\", \"WARN\", \"ERROR\", \"CRITICAL\", \"OFF\" }")

set(SPDLOG_INCLUDE_DIR ${spdlog_SOURCE_DIR}/include PARENT_SCOPE)
set(SPDLOG_LIBRARIES spdlog PARENT_SCOPE)

# ---------------------------------------------------------------------------------------
#     backward-cpp
# ---------------------------------------------------------------------------------------
FetchContent_Declare(backward
    GIT_REPOSITORY https://github.com/bombela/backward-cpp
    GIT_TAG        master  
)

FetchContent_MakeAvailable(backward)

set(BACKWARD_INCLUDE_DIR ${backward_SOURCE_DIR} PARENT_SCOPE)
set(BACKWARD_LIBRARIES Backward::Interface PARENT_SCOPE)

# ---------------------------------------------------------------------------------------
#     glm
# ---------------------------------------------------------------------------------------
set(BUILD_TESTING OFF CACHE BOOL " " FORCE)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        586a402397dd35d66d7a079049856d1e2cbab300
  FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(glm)

if (GLM_FOUND)
  set(GLM_INCLUDE_DIR ${GLM_INCLUDE_DIRS} PARENT_SCOPE)
  set(GLM_LIBRARIES ${GLM_LIBRARIES} PARENT_SCOPE)
else()
  set(GLM_INCLUDE_DIR ${glm_SOURCE_DIR} PARENT_SCOPE)
  set(GLM_LIBRARIES glm PARENT_SCOPE)
endif()