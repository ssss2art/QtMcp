# QtMCP Test Application

set(TEST_APP_SOURCES
    main.cpp
    mainwindow.cpp
)

set(TEST_APP_HEADERS
    mainwindow.h
)

set(TEST_APP_UI
    mainwindow.ui
)

add_executable(qtmcp_test_app
    ${TEST_APP_SOURCES}
    ${TEST_APP_HEADERS}
    ${TEST_APP_UI}
)

target_link_libraries(qtmcp_test_app
    PRIVATE
        Qt5::Core
        Qt5::Widgets
)

# Set output name
set_target_properties(qtmcp_test_app PROPERTIES
    OUTPUT_NAME "qtmcp-test-app"
)

# On Windows, set WIN32 to hide console
if(WIN32)
    set_target_properties(qtmcp_test_app PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
