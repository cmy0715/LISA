cmake_minimum_required(VERSION 3.10)
project(git_interaction_layer)
include(FetchContent)
FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.4.2
)
FetchContent_MakeAvailable(CLI11)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖库
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED libgit2)
find_package(yaml-cpp REQUIRED CONFIG)

# 包含头文件目录
include_directories(${LIBGIT2_INCLUDE_DIRS})

# 链接库目录
link_directories(${LIBGIT2_LIBRARY_DIRS})

# 添加源文件
set(SOURCES
    check_renewing_git.cpp
    local_to_server_git.cpp
    compilation_config.cpp
)

# 创建可执行文件
add_executable(git_interaction
    check_renewing_git.cpp
    local_to_server_git.cpp
    compilation_config.cpp
    main.cpp
)

# 链接libgit2库
target_link_libraries(git_interaction ${LIBGIT2_LIBRARIES} yaml-cpp::yaml-cpp)

# 添加编译选项
target_compile_options(git_interaction PRIVATE -Wall -Wextra -pedantic)