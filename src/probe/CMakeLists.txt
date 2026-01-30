# QtMCP Probe Library

set(PROBE_SOURCES
    core/probe.cpp
    core/object_registry.cpp
    introspection/object_id.cpp
    introspection/variant_json.cpp
    introspection/meta_inspector.cpp
    introspection/signal_monitor.cpp
    interaction/input_simulator.cpp
    transport/websocket_server.cpp
    transport/jsonrpc_handler.cpp
)

set(PROBE_HEADERS
    core/probe.h
    core/injector.h
    core/object_registry.h
    introspection/object_id.h
    introspection/variant_json.h
    introspection/meta_inspector.h
    introspection/signal_monitor.h
    interaction/input_simulator.h
    transport/websocket_server.h
    transport/jsonrpc_handler.h
)

# Platform-specific initialization entry points
# These contain DllMain (Windows) and __attribute__((constructor)) (Linux)
if(WIN32)
    list(APPEND PROBE_SOURCES core/probe_init_windows.cpp)
elseif(UNIX AND NOT APPLE)
    list(APPEND PROBE_SOURCES core/probe_init_linux.cpp)
endif()

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
            Qt6::Test  # For QTest functions in InputSimulator
        PRIVATE
            Qt6::CorePrivate  # For qhooks_p.h (object registry hooks)
    )
else()
    target_link_libraries(qtmcp_probe
        PUBLIC
            Qt5::Core
            Qt5::Network
            Qt5::WebSockets
            Qt5::Widgets
            Qt5::Test  # For QTest functions in InputSimulator
        PRIVATE
            Qt5::CorePrivate  # For qhooks_p.h (object registry hooks)
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
if(UNIX AND NOT APPLE)
    # Add X11 for input simulation (optional, for system-level events)
    find_package(X11)
    if(X11_FOUND AND X11_XTest_FOUND)
        target_link_libraries(qtmcp_probe PRIVATE ${X11_LIBRARIES} ${X11_XTest_LIB})
        target_compile_definitions(qtmcp_probe PRIVATE QTMCP_HAS_XTEST)
    endif()
endif()

# Alias for use in parent project
add_library(QtMCP::probe ALIAS qtmcp_probe)

# Deploy Qt DLLs on Windows (for the probe DLL's dependencies)
qtmcp_deploy_qt(qtmcp_probe)
