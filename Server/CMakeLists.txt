cmake_minimum_required(VERSION 3.10)
project(lisa_server CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -pthread")

# 使用FetchContent获取依赖库
include(FetchContent)

# 获取httplib (HTTP服务器库)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.14.1
)
FetchContent_MakeAvailable(httplib)

# 获取nlohmann/json (JSON库)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# 查找系统依赖库
find_package(PkgConfig REQUIRED)

# 查找libgit2
pkg_check_modules(LIBGIT2 REQUIRED libgit2)
if(NOT LIBGIT2_FOUND)
  message(FATAL_ERROR "libgit2 not found")
endif()

# 使用FetchContent获取yaml-cpp
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG yaml-cpp-0.8.0
)
FetchContent_MakeAvailable(yaml-cpp)

# 包含头文件目录
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LIBGIT2_INCLUDE_DIRS}




)

# 链接库目录
link_directories(
  ${LIBGIT2_LIBRARY_DIRS}
)

# 服务器源文件
set(SERVER_SOURCES
  main.cpp
  server.cpp
  git_handler.cpp
  compilation_handler.cpp
  config.cpp
  logger.cpp
)

# 创建可执行文件
add_executable(lisa_server ${SERVER_SOURCES})

# 链接依赖库
target_link_libraries(lisa_server
  PRIVATE
  ${LIBGIT2_LIBRARIES}
  yaml-cpp::yaml-cpp
  httplib::httplib
  nlohmann_json::nlohmann_json
  pthread
)

# 安装目标
install(TARGETS lisa_server
  RUNTIME DESTINATION bin
)