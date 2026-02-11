# QtMCP Launcher
# Launches Qt applications with the probe library injected.
# Works on both Windows (DLL injection) and Linux (LD_PRELOAD).

set(LAUNCHER_SOURCES
    main.cpp
)

set(LAUNCHER_HEADERS
    injector.h
)

# Platform-specific injection implementation
if(WIN32)
    list(APPEND LAUNCHER_SOURCES injector_windows.cpp)
elseif(UNIX AND NOT APPLE)
    list(APPEND LAUNCHER_SOURCES injector_linux.cpp)
endif()

add_executable(qtmcp_launcher
    ${LAUNCHER_SOURCES}
    ${LAUNCHER_HEADERS}
)

# Link Qt libraries
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
    OUTPUT_NAME "qtmcp-launcher"
    DEBUG_POSTFIX "d"
)

# Include the launcher source directory for injector.h
target_include_directories(qtmcp_launcher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
