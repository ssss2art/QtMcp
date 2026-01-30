# QtMCP - Qt Application Introspection and Automation Library
# Copyright (c) 2024 QtMCP Contributors
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)

project(QtMCP
    VERSION 0.1.0
    DESCRIPTION "Qt application introspection and automation library with MCP integration"
    LANGUAGES CXX
)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy and IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(QTMCP_BUILD_TESTS "Build unit tests" ON)
option(QTMCP_BUILD_TEST_APP "Build the test Qt application" ON)
option(QTMCP_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find Qt - prefer Qt6, fall back to Qt5 5.15+
# This allows building with either Qt version using the same CMakeLists.txt
set(QT_MIN_VERSION_5 "5.15")
set(QT_MIN_VERSION_6 "6.0")

find_package(Qt6 ${QT_MIN_VERSION_6} QUIET COMPONENTS
    Core
    Network
    WebSockets
    Widgets
    Test
)

if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    set(QT_VERSION ${Qt6_VERSION})
    message(STATUS "Found Qt6: ${Qt6_VERSION}")
else()
    find_package(Qt5 ${QT_MIN_VERSION_5} REQUIRED COMPONENTS
        Core
        Network
        WebSockets
        Widgets
        Test
    )
    set(QT_VERSION_MAJOR 5)
    set(QT_VERSION ${Qt5_VERSION})
    message(STATUS "Found Qt5: ${Qt5_VERSION}")
endif()

# Create Qt version-independent target aliases for use throughout the project
# This allows code to link against Qt::Core, Qt::Network, etc. regardless of Qt version
if(QT_VERSION_MAJOR EQUAL 6)
    # Qt6 already provides Qt:: namespace targets
    set(QT_LIBRARIES Qt6::Core Qt6::Network Qt6::WebSockets Qt6::Widgets)
else()
    # Qt5 also provides Qt5:: namespace, create aliases for consistency
    set(QT_LIBRARIES Qt5::Core Qt5::Network Qt5::WebSockets Qt5::Widgets)
endif()

# Enable Qt automoc/autouic/autorcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Optional dependencies - these will be used if available
# Phase 1 uses Qt's built-in JSON (QJsonDocument) and can work without these
find_package(nlohmann_json CONFIG QUIET)
if(nlohmann_json_FOUND)
    message(STATUS "Found nlohmann_json")
    set(QTMCP_HAS_NLOHMANN_JSON ON)
else()
    message(STATUS "nlohmann_json not found - using QJsonDocument")
    set(QTMCP_HAS_NLOHMANN_JSON OFF)
endif()

find_package(spdlog CONFIG QUIET)
if(spdlog_FOUND)
    message(STATUS "Found spdlog")
    set(QTMCP_HAS_SPDLOG ON)
else()
    message(STATUS "spdlog not found - using QDebug for logging")
    set(QTMCP_HAS_SPDLOG OFF)
endif()

# Platform-specific settings
if(WIN32)
    add_definitions(-DQTMCP_PLATFORM_WINDOWS)
    # Disable min/max macros from windows.h
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DQTMCP_PLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DQTMCP_PLATFORM_MACOS)
    message(WARNING "macOS support is not yet implemented")
endif()

# Compiler warnings (Google C++ style encourages treating warnings as errors)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -Wno-unused-parameter
        -Wcast-align
        -Wformat=2
        -Wmissing-include-dirs
        -Wswitch-default
        -Wunused
    )
elseif(MSVC)
    add_compile_options(
        /W4
        /WX
        /permissive-
        /Zc:__cplusplus
    )
endif()

# Clang-tidy integration
if(QTMCP_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy not found, static analysis disabled")
    endif()
endif()

# Add subdirectories
add_subdirectory(src/probe)

if(WIN32)
    add_subdirectory(src/launcher)
endif()

if(QTMCP_BUILD_TEST_APP)
    add_subdirectory(test_app)
endif()

if(QTMCP_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG QUIET)
    if(GTest_FOUND)
        message(STATUS "Found GTest - building unit tests")
        add_subdirectory(tests)
    else()
        message(STATUS "GTest not found - skipping unit tests (use Qt Test instead)")
    endif()
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS qtmcp_probe
    EXPORT QtMCPTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY src/probe/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qtmcp
    FILES_MATCHING PATTERN "*.h"
)

# Package configuration
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QtMCPConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtMCPConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/QtMCPConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtMCP
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/QtMCPConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/QtMCPConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtMCP
)

install(EXPORT QtMCPTargets
    FILE QtMCPTargets.cmake
    NAMESPACE QtMCP::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QtMCP
)

# Print configuration summary
message(STATUS "")
message(STATUS "QtMCP Configuration Summary:")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Build type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "  Qt version:     ${QT_VERSION} (Qt${QT_VERSION_MAJOR})")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Build tests:    ${QTMCP_BUILD_TESTS}")
message(STATUS "  Build test app: ${QTMCP_BUILD_TEST_APP}")
message(STATUS "  Clang-tidy:     ${QTMCP_ENABLE_CLANG_TIDY}")
message(STATUS "")
