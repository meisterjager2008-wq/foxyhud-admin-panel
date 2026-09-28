cmake_minimum_required(VERSION 3.16)
project(foxyhud_admin_panel LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Demos and renderer helpers are on by default only when this is the top-level
# project, so add_subdirectory() from your game just gives you the menu library
# (turn on the FOXYHUD_DXn option for the DirectX version(s) your game uses).
set(FOXYHUD_TOP_LEVEL OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(FOXYHUD_TOP_LEVEL ON)
endif()

set(FOXYHUD_DX_DEFAULT OFF)
set(FOXYHUD_GL_DEFAULT OFF)
if(FOXYHUD_TOP_LEVEL)
    if(WIN32)
        set(FOXYHUD_DX_DEFAULT ON)
    else()
        set(FOXYHUD_GL_DEFAULT ON)
    endif()
endif()

option(FOXYHUD_BUILD_DEMOS "Build a demo program for every enabled renderer" ${FOXYHUD_TOP_LEVEL})
option(FOXYHUD_DX9  "Build DirectX 9 support (foxyhud_dx9)"   ${FOXYHUD_DX_DEFAULT})
option(FOXYHUD_DX10 "Build DirectX 10 support (foxyhud_dx10)" ${FOXYHUD_DX_DEFAULT})
option(FOXYHUD_DX11 "Build DirectX 11 support (foxyhud_dx11)" ${FOXYHUD_DX_DEFAULT})
option(FOXYHUD_DX12 "Build DirectX 12 support (foxyhud_dx12)" ${FOXYHUD_DX_DEFAULT})
option(FOXYHUD_OPENGL_DEMO "Build the GLFW + OpenGL 3 demo" ${FOXYHUD_GL_DEFAULT})

if(NOT WIN32)
    foreach(api DX9 DX10 DX11 DX12)
        if(FOXYHUD_${api})
            message(WARNING "FOXYHUD_${api} needs Windows, turning it off")
            set(FOXYHUD_${api} OFF CACHE BOOL "" FORCE)
        endif()
    endforeach()
endif()

set(FOXYHUD_ANY_DX OFF)
if(FOXYHUD_DX9 OR FOXYHUD_DX10 OR FOXYHUD_DX11 OR FOXYHUD_DX12)
    set(FOXYHUD_ANY_DX ON)
endif()

# Dear ImGui --------------------------------------------------------------
# If your game already defines an `imgui` target, the menu links against it.
# Otherwise ImGui is downloaded and built here. FOXYHUD_IMGUI_DIR points at the
# ImGui source tree; the renderer helpers and demos need its backends/ folder.
set(FOXYHUD_IMGUI_TAG "v1.92.9b" CACHE STRING "Dear ImGui git tag to fetch")
set(FOXYHUD_IMGUI_DIR "" CACHE PATH "Existing Dear ImGui source directory (optional)")

set(FOXYHUD_NEED_BACKENDS OFF)
if(FOXYHUD_ANY_DX OR (FOXYHUD_BUILD_DEMOS AND FOXYHUD_OPENGL_DEMO))
    set(FOXYHUD_NEED_BACKENDS ON)
endif()

include(FetchContent)

if(NOT FOXYHUD_IMGUI_DIR AND (NOT TARGET imgui OR FOXYHUD_NEED_BACKENDS))
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

function(foxyhud_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

# Portable demo .exe files when cross-compiling with MinGW (no extra DLLs).
function(foxyhud_demo_link_options target)
    if(MINGW)
        target_link_options(${target} PRIVATE -static)
    endif()
endfunction()

# The menu (renderer agnostic, only depends on ImGui) -----------------------
add_library(foxyhud_panel STATIC
    src/foxyhud/admin_panel.cpp
    src/foxyhud/page_players.cpp
    src/foxyhud/page_visuals.cpp
    src/foxyhud/page_misc.cpp
    src/foxyhud/page_config.cpp
    src/foxyhud/page_maintenance.cpp
    src/foxyhud/theme.cpp
    src/foxyhud/widgets.cpp
    src/foxyhud/icons.cpp
    src/foxyhud/settings.cpp
    src/foxyhud/panel_util.cpp)
target_include_directories(foxyhud_panel PUBLIC src)
find_package(Threads REQUIRED)
target_link_libraries(foxyhud_panel PUBLIC imgui PRIVATE Threads::Threads)
if(WIN32)
    target_link_libraries(foxyhud_panel PRIVATE shell32)  # ShellExecuteW for "Open Folder"
endif()
foxyhud_warnings(foxyhud_panel)

# DirectX 9 / 10 / 11 / 12 ------------------------------------------------
# foxyhud_dxN = ImGui's Win32 + DirectX N backends + a small helper
# (src/foxyhud/renderers/dxN.h) that sets everything up for the menu.
# ImGui's backend .cpp files get their own targets so our warning flags only
# apply to our code.
function(foxyhud_imgui_backend name)
    if(NOT TARGET imgui_impl_${name})
        add_library(imgui_impl_${name} STATIC ${FOXYHUD_IMGUI_DIR}/backends/imgui_impl_${name}.cpp)
        target_include_directories(imgui_impl_${name} PUBLIC ${FOXYHUD_IMGUI_DIR}/backends)
        target_link_libraries(imgui_impl_${name} PUBLIC imgui ${ARGN})
    endif()
endfunction()

if(FOXYHUD_ANY_DX)
    foxyhud_imgui_backend(win32 dwmapi gdi32)
    add_library(foxyhud_win32 STATIC src/foxyhud/renderers/win32.cpp)
    target_link_libraries(foxyhud_win32 PUBLIC foxyhud_panel imgui_impl_win32)
    foxyhud_warnings(foxyhud_win32)
endif()

function(foxyhud_add_directx api)
    foxyhud_imgui_backend(${api} ${ARGN})
    add_library(foxyhud_${api} STATIC src/foxyhud/renderers/${api}.cpp)
    target_link_libraries(foxyhud_${api} PUBLIC foxyhud_win32 imgui_impl_${api})
    foxyhud_warnings(foxyhud_${api})
    if(FOXYHUD_BUILD_DEMOS)
        add_executable(foxyhud_demo_${api} src/demo/main_${api}.cpp)
        target_include_directories(foxyhud_demo_${api} PRIVATE src/demo)
        target_link_libraries(foxyhud_demo_${api} PRIVATE foxyhud_${api})
        foxyhud_warnings(foxyhud_demo_${api})
        foxyhud_demo_link_options(foxyhud_demo_${api})
    endif()
endfunction()

if(FOXYHUD_DX9)
    foxyhud_add_directx(dx9 d3d9)
endif()
if(FOXYHUD_DX10)
    foxyhud_add_directx(dx10 d3d10 d3dcompiler)
endif()
if(FOXYHUD_DX11)
    foxyhud_add_directx(dx11 d3d11 d3dcompiler)
endif()
if(FOXYHUD_DX12)
    foxyhud_add_directx(dx12 d3d12 dxgi d3dcompiler)
endif()

# OpenGL 3 demo (GLFW) -----------------------------------------------------
if(FOXYHUD_BUILD_DEMOS AND FOXYHUD_OPENGL_DEMO)
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

    add_executable(foxyhud_demo_opengl
        src/demo/main_opengl.cpp
        ${FOXYHUD_IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${FOXYHUD_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp)
    target_include_directories(foxyhud_demo_opengl PRIVATE src/demo ${FOXYHUD_IMGUI_DIR}/backends)
    target_link_libraries(foxyhud_demo_opengl PRIVATE foxyhud_panel glfw OpenGL::GL)
    if(MSVC)
        target_compile_options(foxyhud_demo_opengl PRIVATE /utf-8)
    endif()
endif()
