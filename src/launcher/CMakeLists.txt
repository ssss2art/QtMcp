# QtMCP Windows Launcher
# This executable loads a Qt application with the probe library injected

add_executable(qtmcp_launcher
    main.cpp
)

# Link Qt libraries - use Qt version-independent approach
# QT_VERSION_MAJOR is set by root CMakeLists.txt
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(qtmcp_launcher
        PRIVATE
            Qt6::Core
    )
else()
    target_link_libraries(qtmcp_launcher
        PRIVATE
            Qt5::Core
    )
endif()

set_target_properties(qtmcp_launcher PROPERTIES
    OUTPUT_NAME "qtmcp-launch"
)

# Note: Cannot create ALIAS for executables, so no QtMCP::launcher alias
