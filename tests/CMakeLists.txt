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

# ObjectRegistry test
add_executable(test_object_registry
    test_object_registry.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_object_registry
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Test
    )
else()
    target_link_libraries(test_object_registry
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Test
    )
endif()

target_include_directories(test_object_registry
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_object_registry COMMAND test_object_registry
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_object_registry>"
)

# Set environment variable to disable auto-initialization for isolated testing
set_tests_properties(test_object_registry PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0"
)

if(WIN32)
    add_custom_command(TARGET test_object_registry POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_object_registry>"
        COMMENT "Copying Qt Test DLL for test_object_registry..."
    )
endif()

# Object ID and tree serialization tests
add_executable(test_object_id
    test_object_id.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_object_id
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_object_id
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_object_id
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_object_id COMMAND test_object_id
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_object_id>"
)

# Set environment variables for testing:
# - QTMCP_ENABLED=0 disables auto-initialization for isolated testing
# - QT_QPA_PLATFORM=minimal allows widget tests without display
set_tests_properties(test_object_id PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_object_id POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_object_id>"
        COMMENT "Copying Qt Test DLL for test_object_id..."
    )
endif()

# MetaInspector and variant_json tests
add_executable(test_meta_inspector
    test_meta_inspector.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_meta_inspector
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_meta_inspector
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_meta_inspector
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_meta_inspector COMMAND test_meta_inspector
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_meta_inspector>"
)

# Set environment variables for testing:
# - QTMCP_ENABLED=0 disables auto-initialization for isolated testing
# - QT_QPA_PLATFORM=minimal allows widget tests without display
set_tests_properties(test_meta_inspector PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_meta_inspector POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_meta_inspector>"
        COMMENT "Copying Qt Test DLL for test_meta_inspector..."
    )
endif()

# UI Interaction tests (InputSimulator, Screenshot, HitTest)
add_executable(test_ui_interaction
    test_ui_interaction.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_ui_interaction
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_ui_interaction
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_ui_interaction
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_ui_interaction COMMAND test_ui_interaction
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_ui_interaction>"
)

# Set environment variables for testing:
# - QTMCP_ENABLED=0 disables auto-initialization for isolated testing
# - QT_QPA_PLATFORM=minimal allows widget tests without display
set_tests_properties(test_ui_interaction PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_ui_interaction POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_ui_interaction>"
        COMMENT "Copying Qt Test DLL for test_ui_interaction..."
    )
endif()

# SignalMonitor tests
add_executable(test_signal_monitor
    test_signal_monitor.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_signal_monitor
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_signal_monitor
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_signal_monitor
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_signal_monitor COMMAND test_signal_monitor
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_signal_monitor>"
)

# Set environment variables for testing:
# - QTMCP_ENABLED=0 disables auto-initialization for isolated testing
# - QT_QPA_PLATFORM=minimal allows widget tests without display
set_tests_properties(test_signal_monitor PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_signal_monitor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_signal_monitor>"
        COMMENT "Copying Qt Test DLL for test_signal_monitor..."
    )
endif()
