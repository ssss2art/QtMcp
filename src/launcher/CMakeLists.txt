# QtMCP Windows Launcher
# This executable loads a Qt application with the probe library injected

add_executable(qtmcp_launcher
    main.cpp
)

target_link_libraries(qtmcp_launcher
    PRIVATE
        Qt5::Core
)

set_target_properties(qtmcp_launcher PROPERTIES
    OUTPUT_NAME "qtmcp-launcher"
)
