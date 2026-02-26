# QtMCP Test Application

set(TEST_APP_SOURCES
    main.cpp
    mainwindow.cpp
    childwindow.cpp
)

set(TEST_APP_HEADERS
    mainwindow.h
    childwindow.h
)

set(TEST_APP_UI
    mainwindow.ui
)

add_executable(qtmcp_test_app
    ${TEST_APP_SOURCES}
    ${TEST_APP_HEADERS}
    ${TEST_APP_UI}
)

# Link Qt libraries
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(qtmcp_test_app
        PRIVATE
            Qt6::Core
            Qt6::Widgets
    )
else()
    target_link_libraries(qtmcp_test_app
        PRIVATE
            Qt5::Core
            Qt5::Widgets
    )
endif()

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
