# QtMCP Shared Utilities
# Windows-only static library with process injection helpers.

add_library(qtmcp_shared STATIC
    process_inject_windows.cpp
    process_inject_windows.h
)

target_include_directories(qtmcp_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Win32 API only (no Qt dependency)
target_link_libraries(qtmcp_shared PRIVATE Psapi)
