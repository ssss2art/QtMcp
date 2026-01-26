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
    OUTPUT_NAME "qtmcp"
    EXPORT_NAME probe
)

# Define export macro for Windows DLL
target_compile_definitions(qtmcp_probe PRIVATE
    QTMCP_PROBE_LIBRARY
)

# Include directories
target_include_directories(qtmcp_probe
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include/qtmcp>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(qtmcp_probe
    PUBLIC
        Qt5::Core
        Qt5::Widgets
        Qt5::WebSockets
    PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

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
