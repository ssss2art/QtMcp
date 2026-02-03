# portfile.cmake for qtmcp-bin
# Downloads prebuilt QtMCP probe from GitHub Releases.
# Detects installed Qt version and downloads only the matching probe binary.

# ---------------------------------------------------------------------------
# Detect installed Qt version
# ---------------------------------------------------------------------------
find_package(Qt6 QUIET COMPONENTS Core)
if(Qt6_FOUND)
    set(_qt_version "${Qt6_VERSION}")
else()
    find_package(Qt5 5.15 QUIET COMPONENTS Core)
    if(Qt5_FOUND)
        set(_qt_version "${Qt5_VERSION}")
    else()
        message(FATAL_ERROR
            "qtmcp-bin requires Qt to be installed (Qt 5.15+ or Qt 6.5+). "
            "Install Qt first, then retry. Or use the 'qtmcp' source port instead.")
    endif()
endif()

# Extract major.minor tag (e.g., "5.15", "6.5", "6.8", "6.9")
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _qt_mm "${_qt_version}")

# ---------------------------------------------------------------------------
# Available prebuilt Qt versions
# ---------------------------------------------------------------------------
# NOTE: Qt 6.2 was dropped in Phase 11.1 (source compatibility refactor).
#       This aligns with the CI matrix which builds only these versions.
set(_available_versions "5.15;6.5;6.8;6.9")

if(NOT "${_qt_mm}" IN_LIST _available_versions)
    message(FATAL_ERROR
        "qtmcp-bin: No prebuilt probe available for Qt ${_qt_mm}. "
        "Available versions: ${_available_versions}. "
        "Use the 'qtmcp' source port to build against Qt ${_qt_mm}.")
endif()

set(_release_tag "v${VERSION}")
set(_qt_tag "qt${_qt_mm}")

# ---------------------------------------------------------------------------
# Determine platform and artifact details
# ---------------------------------------------------------------------------
if(VCPKG_TARGET_IS_WINDOWS)
    set(_platform "windows-msvc")
    set(_ext "dll")
    set(_lib_ext "lib")
    set(_prefix "")
elseif(VCPKG_TARGET_IS_LINUX)
    set(_platform "linux-gcc")
    set(_ext "so")
    set(_prefix "lib")
else()
    message(FATAL_ERROR "qtmcp-bin: Unsupported platform. Only Windows and Linux are supported.")
endif()

# ---------------------------------------------------------------------------
# SHA512 hashes for each artifact
# ---------------------------------------------------------------------------
# PLACEHOLDER values - these must be updated when a real release is created.
# Format: SHA512_qt{M}.{m}_{platform}_{ext}
#
# To calculate: sha512sum qtmcp-probe-qt6.9-linux-gcc.so | awk '{print $1}'
#
# Windows DLLs
set(SHA512_qt5.15_windows-msvc_dll "0")
set(SHA512_qt6.5_windows-msvc_dll "0")
set(SHA512_qt6.8_windows-msvc_dll "0")
set(SHA512_qt6.9_windows-msvc_dll "0")
# Windows import libraries
set(SHA512_qt5.15_windows-msvc_lib "0")
set(SHA512_qt6.5_windows-msvc_lib "0")
set(SHA512_qt6.8_windows-msvc_lib "0")
set(SHA512_qt6.9_windows-msvc_lib "0")
# Linux shared objects
set(SHA512_qt5.15_linux-gcc_so "0")
set(SHA512_qt6.5_linux-gcc_so "0")
set(SHA512_qt6.8_linux-gcc_so "0")
set(SHA512_qt6.9_linux-gcc_so "0")

# ---------------------------------------------------------------------------
# Download ONLY the single probe binary for the detected Qt version
# ---------------------------------------------------------------------------
# Artifact naming from release.yml: qtmcp-probe-qt{M}.{m}-{platform}.{ext}
set(_artifact_name "qtmcp-probe-${_qt_tag}-${_platform}.${_ext}")
set(_download_url "https://github.com/ssss2art/QtMcp/releases/download/${_release_tag}/${_artifact_name}")

set(_hash_var "SHA512_${_qt_tag}_${_platform}_${_ext}")
set(_hash "${${_hash_var}}")

vcpkg_download_distfile(PROBE_BINARY
    URLS "${_download_url}"
    FILENAME "${_artifact_name}"
    SHA512 ${_hash}
)

# Also download import library for Windows
if(VCPKG_TARGET_IS_WINDOWS)
    set(_lib_artifact "qtmcp-probe-${_qt_tag}-${_platform}.${_lib_ext}")
    set(_lib_url "https://github.com/ssss2art/QtMcp/releases/download/${_release_tag}/${_lib_artifact}")

    set(_lib_hash_var "SHA512_${_qt_tag}_${_platform}_${_lib_ext}")
    set(_lib_hash "${${_lib_hash_var}}")

    vcpkg_download_distfile(PROBE_IMPLIB
        URLS "${_lib_url}"
        FILENAME "${_lib_artifact}"
        SHA512 ${_lib_hash}
    )
endif()

# ---------------------------------------------------------------------------
# Install probe binary to lib/qtmcp/qt{M}.{m}/
# ---------------------------------------------------------------------------
# This matches the source port's install layout EXACTLY:
#   Linux:   lib/qtmcp/qt{M}.{m}/libqtmcp-probe-qt{M}.{m}.so
#   Windows: lib/qtmcp/qt{M}.{m}/qtmcp-probe-qt{M}.{m}.dll
set(_install_libdir "lib/qtmcp/${_qt_tag}")

file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/${_install_libdir}")

# Rename from release artifact name to standard installed name
# Release:   qtmcp-probe-qt6.9-linux-gcc.so
# Installed: libqtmcp-probe-qt6.9.so (Linux) or qtmcp-probe-qt6.9.dll (Windows)
file(INSTALL "${PROBE_BINARY}" DESTINATION "${CURRENT_PACKAGES_DIR}/${_install_libdir}"
     RENAME "${_prefix}qtmcp-probe-${_qt_tag}.${_ext}")

if(VCPKG_TARGET_IS_WINDOWS AND DEFINED PROBE_IMPLIB)
    file(INSTALL "${PROBE_IMPLIB}" DESTINATION "${CURRENT_PACKAGES_DIR}/${_install_libdir}"
         RENAME "qtmcp-probe-${_qt_tag}.${_lib_ext}")
endif()

# ---------------------------------------------------------------------------
# Install CMake config so find_package(QtMCP) works
# ---------------------------------------------------------------------------
file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/share/qtmcp")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/QtMCPConfig.cmake"
    "${CURRENT_PACKAGES_DIR}/share/qtmcp/QtMCPConfig.cmake"
    @ONLY
)

# Generate version file
file(WRITE "${CURRENT_PACKAGES_DIR}/share/qtmcp/QtMCPConfigVersion.cmake"
"set(PACKAGE_VERSION \"${VERSION}\")
set(PACKAGE_VERSION_EXACT FALSE)
set(PACKAGE_VERSION_COMPATIBLE FALSE)
if(PACKAGE_FIND_VERSION_MAJOR STREQUAL \"0\")
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    if(PACKAGE_FIND_VERSION STREQUAL PACKAGE_VERSION)
        set(PACKAGE_VERSION_EXACT TRUE)
    endif()
endif()
")

# ---------------------------------------------------------------------------
# Install usage instructions
# ---------------------------------------------------------------------------
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

# ---------------------------------------------------------------------------
# Install license
# ---------------------------------------------------------------------------
vcpkg_download_distfile(LICENSE_FILE
    URLS "https://raw.githubusercontent.com/ssss2art/QtMcp/main/LICENSE"
    FILENAME "qtmcp-LICENSE"
    SHA512 0
)
vcpkg_install_copyright(FILE_LIST "${LICENSE_FILE}")

# ---------------------------------------------------------------------------
# vcpkg policies for binary-only port
# ---------------------------------------------------------------------------
# No import library on Linux (shared object only)
set(VCPKG_POLICY_DLLS_WITHOUT_LIBS enabled)
# No headers - probe is loaded via injection, not linked directly
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
