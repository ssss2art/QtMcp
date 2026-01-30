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
add_test(NAME test_jsonrpc COMMAND test_jsonrpc)
