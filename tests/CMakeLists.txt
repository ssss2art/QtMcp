# QtMCP Tests

# Test sources
set(TEST_SOURCES
    test_jsonrpc.cpp
)

# Create test executable
add_executable(qtmcp_tests
    ${TEST_SOURCES}
)

target_link_libraries(qtmcp_tests
    PRIVATE
        qtmcp_probe
        Qt5::Core
        Qt5::Test
        GTest::gtest
        GTest::gtest_main
        nlohmann_json::nlohmann_json
)

target_include_directories(qtmcp_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(qtmcp_tests)
