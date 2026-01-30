# QtMCP Probe Library

set(PROBE_SOURCES
    core/probe.cpp
    core/injector_linux.cpp
    core/injector_windows.cpp
    transport/websocket_server.cpp
    transport/jsonrpc_handler.cpp
)

set(PROBE_HEADERS
    core/probe.h
    core/injector.h
    transport/websocket_server.h
    transport/jsonrpc_handler.h
)

# Create shared library
add_library(qtmcp_probe SHARED
    ${PROBE_SOURCES}
    ${PROBE_HEADERS}
)

# Set library properties
set_target_properties(qtmcp_probe PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "qtmcp-probe"
    EXPORT_NAME probe
)

# Define export macro for Windows DLL
target_compile_definitions(qtmcp_probe PRIVATE
    QTMCP_PROBE_LIBRARY
)

# Windows: export all symbols for DLL (simplifies development)
if(WIN32)
    set_target_properties(qtmcp_probe PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
    )
endif()

# Include directories
target_include_directories(qtmcp_probe
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/qtmcp>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link Qt libraries - use Qt version-independent approach
# QT_VERSION_MAJOR is set by root CMakeLists.txt
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(qtmcp_probe
        PUBLIC
            Qt6::Core
            Qt6::Network
            Qt6::WebSockets
            Qt6::Widgets
    )
else()
    target_link_libraries(qtmcp_probe
        PUBLIC
            Qt5::Core
            Qt5::Network
            Qt5::WebSockets
            Qt5::Widgets
    )
endif()

# Optional dependencies - conditionally link if available
if(QTMCP_HAS_NLOHMANN_JSON)
    target_link_libraries(qtmcp_probe PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(qtmcp_probe PRIVATE QTMCP_HAS_NLOHMANN_JSON)
endif()

if(QTMCP_HAS_SPDLOG)
    target_link_libraries(qtmcp_probe PRIVATE spdlog::spdlog)
    target_compile_definitions(qtmcp_probe PRIVATE QTMCP_HAS_SPDLOG)
endif()

# Platform-specific configurations
if(WIN32)
    # Windows-specific: exclude Linux injector from compilation
    set_source_files_properties(
        core/injector_linux.cpp
        PROPERTIES HEADER_FILE_ONLY TRUE
    )
elseif(UNIX AND NOT APPLE)
    # Linux-specific: exclude Windows injector from compilation
    set_source_files_properties(
        core/injector_windows.cpp
        PROPERTIES HEADER_FILE_ONLY TRUE
    )

    # Add X11 for input simulation (optional, for system-level events)
    find_package(X11)
    if(X11_FOUND AND X11_XTest_FOUND)
        target_link_libraries(qtmcp_probe PRIVATE ${X11_LIBRARIES} ${X11_XTest_LIB})
        target_compile_definitions(qtmcp_probe PRIVATE QTMCP_HAS_XTEST)
    endif()
endif()

# Alias for use in parent project
add_library(QtMCP::probe ALIAS qtmcp_probe)
