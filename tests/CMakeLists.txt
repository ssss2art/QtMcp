# QtMCP Tests using Qt Test framework

enable_testing()

# Deploy the minimal platform plugin for headless testing (QT_QPA_PLATFORM=minimal).
# windeployqt only deploys 'qwindows' — we need 'qminimal' explicitly.
if(WIN32)
    get_target_property(_qt_plugins_dir Qt${QT_VERSION_MAJOR}::Core LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_plugins_dir}" DIRECTORY)
    set(_qminimal_src "${_qt_bin_dir}/../plugins/platforms/qminimald.dll")
    if(EXISTS "${_qminimal_src}")
        # Copy once via the first test target (all tests share the same output dir)
        add_custom_command(OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/platforms/qminimald.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qminimal_src}"
                "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/platforms/qminimald.dll"
            COMMENT "Deploying qminimald.dll for headless tests..."
        )
        add_custom_target(deploy_qminimal ALL
            DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/platforms/qminimald.dll"
        )
    endif()
endif()

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

# JSON-RPC Introspection Integration tests
add_executable(test_jsonrpc_introspection
    test_jsonrpc_introspection.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_jsonrpc_introspection
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_jsonrpc_introspection
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_jsonrpc_introspection
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_jsonrpc_introspection COMMAND test_jsonrpc_introspection
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_jsonrpc_introspection>"
)

# Set environment variables for testing:
# - QTMCP_ENABLED=0 disables auto-initialization for isolated testing
# - QT_QPA_PLATFORM=minimal allows widget tests without display
set_tests_properties(test_jsonrpc_introspection PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_jsonrpc_introspection POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_jsonrpc_introspection>"
        COMMENT "Copying Qt Test DLL for test_jsonrpc_introspection..."
    )
endif()

# Native Mode API Integration tests
add_executable(test_native_mode_api
    test_native_mode_api.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_native_mode_api
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_native_mode_api
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_native_mode_api
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_native_mode_api COMMAND test_native_mode_api
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_native_mode_api>"
)

# Set environment variables for testing:
# - QTMCP_ENABLED=0 disables auto-initialization for isolated testing
# - QT_QPA_PLATFORM=minimal allows widget tests without display
set_tests_properties(test_native_mode_api PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_native_mode_api POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_native_mode_api>"
        COMMENT "Copying Qt Test DLL for test_native_mode_api..."
    )
endif()

# KeyNameMapper unit tests
add_executable(test_key_name_mapper
    test_key_name_mapper.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_key_name_mapper
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_key_name_mapper
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_key_name_mapper
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_key_name_mapper COMMAND test_key_name_mapper
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_key_name_mapper>"
)

set_tests_properties(test_key_name_mapper PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_key_name_mapper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_key_name_mapper>"
        COMMENT "Copying Qt Test DLL for test_key_name_mapper..."
    )
endif()

# Computer Use Mode API Integration tests
add_executable(test_computer_use_api
    test_computer_use_api.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_computer_use_api
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_computer_use_api
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_computer_use_api
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_computer_use_api COMMAND test_computer_use_api
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_computer_use_api>"
)

set_tests_properties(test_computer_use_api PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_computer_use_api POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_computer_use_api>"
        COMMENT "Copying Qt Test DLL for test_computer_use_api..."
    )
endif()

# Chrome Mode API Integration tests
add_executable(test_chrome_mode_api
    test_chrome_mode_api.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_chrome_mode_api
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_chrome_mode_api
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_chrome_mode_api
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_chrome_mode_api COMMAND test_chrome_mode_api
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_chrome_mode_api>"
)

set_tests_properties(test_chrome_mode_api PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_chrome_mode_api POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_chrome_mode_api>"
        COMMENT "Copying Qt Test DLL for test_chrome_mode_api..."
    )
endif()

# Model Navigator / QML Inspect Integration tests
add_executable(test_model_navigator
    test_model_navigator.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_model_navigator
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_model_navigator
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_model_navigator
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_model_navigator COMMAND test_model_navigator
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_model_navigator>"
)

set_tests_properties(test_model_navigator PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_model_navigator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_model_navigator>"
        COMMENT "Copying Qt Test DLL for test_model_navigator..."
    )
endif()

# EventCapture window lifecycle event tests
add_executable(test_event_capture
    test_event_capture.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(test_event_capture
        PRIVATE
            qtmcp_probe
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
            Qt6::Test
    )
else()
    target_link_libraries(test_event_capture
        PRIVATE
            qtmcp_probe
            Qt5::Core
            Qt5::Gui
            Qt5::Widgets
            Qt5::Test
    )
endif()

target_include_directories(test_event_capture
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/probe
)

add_test(NAME test_event_capture COMMAND test_event_capture
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:test_event_capture>"
)

set_tests_properties(test_event_capture PROPERTIES
    ENVIRONMENT "QTMCP_ENABLED=0;QT_QPA_PLATFORM=minimal"
)

if(WIN32)
    add_custom_command(TARGET test_event_capture POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::Test>"
            "$<TARGET_FILE_DIR:test_event_capture>"
        COMMENT "Copying Qt Test DLL for test_event_capture..."
    )
endif()
