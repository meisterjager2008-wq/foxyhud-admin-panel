cmake_minimum_required(VERSION 3.16)
project(foxyhud_admin_panel LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The demo is built by default only when this is the top-level project, so
# add_subdirectory() from your game just gives you the library.
set(FOXYHUD_TOP_LEVEL OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(FOXYHUD_TOP_LEVEL ON)
endif()
option(FOXYHUD_BUILD_DEMO "Build the standalone GLFW + OpenGL3 demo app" ${FOXYHUD_TOP_LEVEL})

# Dear ImGui --------------------------------------------------------------
# If your game already defines an `imgui` target, the panel links against it.
# Otherwise ImGui is downloaded and built here. FOXYHUD_IMGUI_DIR points at the
# ImGui source tree (needed by the demo for the platform/renderer backends).
set(FOXYHUD_IMGUI_TAG "v1.92.9b" CACHE STRING "Dear ImGui git tag to fetch")
set(FOXYHUD_IMGUI_DIR "" CACHE PATH "Existing Dear ImGui source directory (optional)")

include(FetchContent)

if(NOT FOXYHUD_IMGUI_DIR AND (NOT TARGET imgui OR FOXYHUD_BUILD_DEMO))
    FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        ${FOXYHUD_IMGUI_TAG}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(imgui)
    set(FOXYHUD_IMGUI_DIR ${imgui_SOURCE_DIR})
endif()

if(NOT TARGET imgui)
    add_library(imgui STATIC
        ${FOXYHUD_IMGUI_DIR}/imgui.cpp
        ${FOXYHUD_IMGUI_DIR}/imgui_draw.cpp
        ${FOXYHUD_IMGUI_DIR}/imgui_tables.cpp
        ${FOXYHUD_IMGUI_DIR}/imgui_widgets.cpp
        ${FOXYHUD_IMGUI_DIR}/imgui_demo.cpp)
    target_include_directories(imgui PUBLIC ${FOXYHUD_IMGUI_DIR})
endif()

# Admin panel library (renderer agnostic, only depends on ImGui) ----------
add_library(foxyhud_panel STATIC
    src/foxyhud/admin_panel.cpp
    src/foxyhud/page_players.cpp
    src/foxyhud/page_misc.cpp
    src/foxyhud/page_config.cpp
    src/foxyhud/page_maintenance.cpp
    src/foxyhud/overlay_windows.cpp
    src/foxyhud/theme.cpp
    src/foxyhud/widgets.cpp
    src/foxyhud/icons.cpp
    src/foxyhud/settings.cpp
    src/foxyhud/panel_util.cpp)
target_include_directories(foxyhud_panel PUBLIC src)
target_link_libraries(foxyhud_panel PUBLIC imgui)

if(MSVC)
    target_compile_options(foxyhud_panel PRIVATE /W4 /utf-8)
else()
    target_compile_options(foxyhud_panel PRIVATE -Wall -Wextra)
endif()

# Demo -------------------------------------------------------------------
if(FOXYHUD_BUILD_DEMO)
    find_package(OpenGL REQUIRED)

    find_package(glfw3 3.3 QUIET)
    if(NOT glfw3_FOUND)
        set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
        if(UNIX AND NOT APPLE)
            # GLFW 3.4 builds Wayland support by default; skip it when the tools are missing.
            find_program(FOXYHUD_WAYLAND_SCANNER wayland-scanner)
            if(NOT FOXYHUD_WAYLAND_SCANNER)
                set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
            endif()
        endif()
        FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG        3.4
            GIT_SHALLOW    TRUE)
        FetchContent_MakeAvailable(glfw)
    endif()

    add_executable(foxyhud_demo
        src/demo/main.cpp
        src/demo/mock_backend.cpp
        ${FOXYHUD_IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${FOXYHUD_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
    target_include_directories(foxyhud_demo PRIVATE ${FOXYHUD_IMGUI_DIR}/backends)
    target_link_libraries(foxyhud_demo PRIVATE foxyhud_panel glfw OpenGL::GL)
    if(MSVC)
        target_compile_options(foxyhud_demo PRIVATE /utf-8)
    endif()
endif()
