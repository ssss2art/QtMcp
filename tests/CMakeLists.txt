# QtMCP Tests using Qt Test framework

enable_testing()

# Test sources
set(TEST_SOURCES
    test_jsonrpc.cpp
)

# Create test executable
add_executable(test_jsonrpc
    ${TEST_SOURCES}
)

# Link Qt libraries - use Qt version-independent approach
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_jsonrpc
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Test
    )
else()
    target_link_libraries(test_jsonrpc
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Test
    )
endif()

target_include_directories(test_jsonrpc
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

# Register test with CTest
# Set working directory to where DLLs are deployed (bin/Debug on MSVC)
add_test(NAME test_jsonrpc COMMAND test_jsonrpc
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_jsonrpc>"
)

# On Windows, Qt DLLs are needed for tests to run.
# Other targets (probe, launcher) already deploy common Qt DLLs via windeployqt.
# For tests, we only need to copy Qt Test DLL which isn't auto-detected.
if(WIN32)
    # Copy Qt Test DLL - not auto-detected by windeployqt
    add_custom_command(TARGET test_jsonrpc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_jsonrpc>"
        COMMENT "Copying Qt Test DLL for test_jsonrpc..."
    )
endif()
