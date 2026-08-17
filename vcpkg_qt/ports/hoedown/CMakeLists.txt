cmake_minimum_required(VERSION 3.16)

project(hoedown VERSION 3.0.7 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Hoedown does not annotate its API for DLL exports, so always package it as a
# static library. It has no runtime dependencies beyond the C runtime.
add_library(hoedown STATIC
    src/autolink.c
    src/buffer.c
    src/document.c
    src/escape.c
    src/html.c
    src/html_blocks.c
    src/html_smartypants.c
    src/stack.c
    src/version.c
)
add_library(hoedown::hoedown ALIAS hoedown)

target_include_directories(hoedown PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hoedown>
)

install(TARGETS hoedown EXPORT hoedownTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(DIRECTORY src/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/hoedown"
    FILES_MATCHING PATTERN "*.h"
)
install(EXPORT hoedownTargets
    FILE hoedownTargets.cmake
    NAMESPACE hoedown::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/hoedown"
)

configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/hoedownConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hoedownConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/hoedown"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hoedownConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hoedownConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hoedownConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/hoedown"
)
