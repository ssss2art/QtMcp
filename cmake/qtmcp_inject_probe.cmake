# qtmcp_inject_probe.cmake
# ---------------------------------------------------------------------------
# Helper function for downstream consumers to inject the QtMCP probe
# into their Qt application targets.
#
# Usage:
#   find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
#   find_package(QtMCP REQUIRED)
#
#   add_executable(myapp main.cpp)
#   target_link_libraries(myapp PRIVATE Qt6::Core Qt6::Widgets)
#   qtmcp_inject_probe(myapp)
#
# What it does:
#   - Links QtMCP::Probe to the target (PRIVATE)
#   - On Windows: copies the probe DLL next to the target executable
#   - On Linux: generates a helper script for LD_PRELOAD injection
# ---------------------------------------------------------------------------

function(qtmcp_inject_probe TARGET_NAME)
    if(NOT TARGET QtMCP::Probe)
        message(FATAL_ERROR
            "qtmcp_inject_probe: QtMCP::Probe target not found. "
            "Call find_package(QtMCP) first.")
    endif()

    if(NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR
            "qtmcp_inject_probe: Target '${TARGET_NAME}' does not exist.")
    endif()

    # Link the probe library
    target_link_libraries(${TARGET_NAME} PRIVATE QtMCP::Probe)

    # On Windows, copy the probe DLL next to the target executable
    if(WIN32)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:QtMCP::Probe>"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>"
            COMMENT "Copying QtMCP probe DLL for ${TARGET_NAME}"
            VERBATIM
        )
    endif()

    # On Linux, generate a helper script for LD_PRELOAD-based injection
    if(UNIX AND NOT APPLE)
        get_target_property(_probe_location QtMCP::Probe IMPORTED_LOCATION)
        if(_probe_location)
            set(_preload_script "${CMAKE_CURRENT_BINARY_DIR}/qtmcp-preload-${TARGET_NAME}.sh")
            file(GENERATE OUTPUT "${_preload_script}"
                CONTENT "#!/bin/sh\nLD_PRELOAD=$<TARGET_FILE:QtMCP::Probe> exec $<TARGET_FILE:${TARGET_NAME}> \"$@\"\n"
            )
        endif()
    endif()
endfunction()
