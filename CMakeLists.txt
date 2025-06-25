cmake_minimum_required(VERSION 3.10)
project(LISA VERSION 1.0.0)

# 包含子项目
add_subdirectory(Client)
add_subdirectory(Server)

# 启用CPack打包功能
include(CPack)

# 基本打包配置
set(CPACK_PACKAGE_NAME "LISA")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Linked Interactive Source Automator")
set(CPACK_PACKAGE_CONTACT "user@example.com")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

# 平台特定打包配置
if(UNIX AND NOT APPLE)
    # Linux配置
    set(CPACK_GENERATOR "DEB" "RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgit2-dev, libyaml-cpp-dev, g++")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Maintainer")
    set(CPACK_RPM_PACKAGE_REQUIRES "libgit2-devel, yaml-cpp-devel")
elif(APPLE)
    # macOS配置
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "LISA Installer")
    set(CPACK_DMG_FORMAT "UDBZ")
elif(WIN32)
    # Windows配置
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "LISA")
    set(CPACK_NSIS_CONTACT "user@example.com")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
endif()