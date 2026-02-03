# QtMCPConfig.cmake
# Installed by qtmcp-bin vcpkg port.
# Detects consumer's Qt version and creates QtMCP::Probe IMPORTED target.

include(CMakeFindDependencyMacro)

# ---------------------------------------------------------------------------
# Detect Qt version
# ---------------------------------------------------------------------------
# The consumer should have already called find_package(Qt6 ...) or find_package(Qt5 ...)
if(TARGET Qt6::Core)
    set(_qtmcp_qt_major 6)
    get_target_property(_qt_version Qt6::Core VERSION)
    if(NOT _qt_version)
        set(_qt_version "${Qt6Core_VERSION}")
    endif()
elseif(TARGET Qt5::Core)
    set(_qtmcp_qt_major 5)
    get_target_property(_qt_version Qt5::Core VERSION)
    if(NOT _qt_version)
        set(_qt_version "${Qt5Core_VERSION}")
    endif()
else()
    # Try finding Qt ourselves
    find_package(Qt6 QUIET COMPONENTS Core)
    if(Qt6_FOUND)
        set(_qtmcp_qt_major 6)
        set(_qt_version "${Qt6_VERSION}")
    else()
        find_package(Qt5 5.15 QUIET COMPONENTS Core)
        if(Qt5_FOUND)
            set(_qtmcp_qt_major 5)
            set(_qt_version "${Qt5_VERSION}")
        else()
            set(QtMCP_FOUND FALSE)
            set(QtMCP_NOT_FOUND_MESSAGE
                "QtMCP requires Qt5 (5.15+) or Qt6. No Qt found. "
                "Call find_package(Qt6) or find_package(Qt5) before find_package(QtMCP).")
            return()
        endif()
    endif()
endif()

# Extract major.minor for versioned path lookup (e.g., "qt6.9")
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _qtmcp_qt_mm "${_qt_version}")
set(_qtmcp_qt_tag "qt${_qtmcp_qt_mm}")

# ---------------------------------------------------------------------------
# Find Qt dependencies that QtMCP needs (consumer must have these)
# ---------------------------------------------------------------------------
if(_qtmcp_qt_major EQUAL 6)
    find_dependency(Qt6 COMPONENTS Core Network WebSockets Widgets)
else()
    find_dependency(Qt5 5.15 COMPONENTS Core Network WebSockets Widgets)
endif()

# ---------------------------------------------------------------------------
# Resolve library directory relative to this config file
# ---------------------------------------------------------------------------
# Config is installed at: <prefix>/share/qtmcp/QtMCPConfig.cmake
# Libraries are at:       <prefix>/lib/qtmcp/<qt-version-tag>/
get_filename_component(_qtmcp_prefix "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(_qtmcp_lib_dir "${_qtmcp_prefix}/lib/qtmcp/${_qtmcp_qt_tag}")

# Verify the versioned library directory exists
if(NOT EXISTS "${_qtmcp_lib_dir}")
    set(QtMCP_FOUND FALSE)
    set(QtMCP_NOT_FOUND_MESSAGE
        "QtMCP: no probe found for ${_qtmcp_qt_tag}. "
        "Available builds: check ${_qtmcp_prefix}/lib/qtmcp/")
    return()
endif()

# ---------------------------------------------------------------------------
# Create imported target: QtMCP::Probe
# ---------------------------------------------------------------------------
if(NOT TARGET QtMCP::Probe)
    add_library(QtMCP::Probe SHARED IMPORTED)

    # Library base name matches the build output: qtmcp-probe-qt{M}.{m}
    set(_probe_base "qtmcp-probe-${_qtmcp_qt_tag}")

    if(WIN32)
        # Windows: DLL and import lib in the versioned lib dir
        find_file(_probe_dll
            NAMES "${_probe_base}.dll"
            PATHS "${_qtmcp_lib_dir}"
            NO_DEFAULT_PATH
        )
        find_file(_probe_implib
            NAMES "${_probe_base}.lib" "${_probe_base}.dll.a"
            PATHS "${_qtmcp_lib_dir}"
            NO_DEFAULT_PATH
        )
        if(_probe_dll)
            set_target_properties(QtMCP::Probe PROPERTIES
                IMPORTED_LOCATION "${_probe_dll}"
            )
            if(_probe_implib)
                set_target_properties(QtMCP::Probe PROPERTIES
                    IMPORTED_IMPLIB "${_probe_implib}"
                )
            endif()
        endif()
    else()
        # Linux/Unix: shared object in the versioned lib dir
        find_file(_probe_so
            NAMES "lib${_probe_base}.so"
            PATHS "${_qtmcp_lib_dir}"
            NO_DEFAULT_PATH
        )
        if(_probe_so)
            set_target_properties(QtMCP::Probe PROPERTIES
                IMPORTED_LOCATION "${_probe_so}"
            )
        endif()
    endif()

    # Link Qt dependencies on the imported target
    if(_qtmcp_qt_major EQUAL 6)
        set_target_properties(QtMCP::Probe PROPERTIES
            INTERFACE_LINK_LIBRARIES "Qt6::Core;Qt6::Network;Qt6::WebSockets;Qt6::Widgets"
        )
    else()
        set_target_properties(QtMCP::Probe PROPERTIES
            INTERFACE_LINK_LIBRARIES "Qt5::Core;Qt5::Network;Qt5::WebSockets;Qt5::Widgets"
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Clean up temporary variables
# ---------------------------------------------------------------------------
unset(_qtmcp_qt_major)
unset(_qt_version)
unset(_qtmcp_qt_mm)
unset(_qtmcp_qt_tag)
unset(_qtmcp_prefix)
unset(_qtmcp_lib_dir)
unset(_probe_base)
unset(_probe_dll CACHE)
unset(_probe_implib CACHE)
unset(_probe_so CACHE)
