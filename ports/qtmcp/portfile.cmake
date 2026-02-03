vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ssss2art/QtMcp
    REF "v${VERSION}"
    SHA512 0  # Placeholder - update with actual hash when first release tag is created
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DQTMCP_BUILD_TESTS=OFF
        -DQTMCP_BUILD_TEST_APP=OFF
        -DQTMCP_ENABLE_CLANG_TIDY=OFF
        -DQTMCP_DEPLOY_QT=OFF
)

vcpkg_cmake_install()

# Fix up the CMake config files
# Project installs to: share/cmake/QtMCP/
# vcpkg expects:       share/qtmcp/
vcpkg_cmake_config_fixup(PACKAGE_NAME QtMCP CONFIG_PATH share/cmake/QtMCP)

# Remove empty include directory if present (probe is loaded via injection, headers not typically needed)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Remove duplicate debug share directory
if(EXISTS "${CURRENT_PACKAGES_DIR}/debug/share")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
endif()

# Install usage instructions
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

# Install license
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
