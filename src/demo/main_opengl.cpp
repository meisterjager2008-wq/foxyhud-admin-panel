// OpenGL 3 demo (Windows / Linux / macOS): a window with the menu and nothing else.
// Press INSERT to toggle the menu.

#include <cstdio>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "demo_common.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace {

void GlfwError(int code, const char* description) { std::fprintf(stderr, "GLFW error %d: %s\n", code, description); }

using PFN_ClearColor = void (*)(float, float, float, float);
using PFN_Clear = void (*)(unsigned int);
constexpr unsigned int kColorBufferBit = 0x00004000;  // GL_COLOR_BUFFER_BIT

} // namespace

int main(int, char**) {
    glfwSetErrorCallback(GlfwError);
    if (!glfwInit())
        return 1;

#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    // Scale the UI with the monitor's DPI setting (e.g. 150% on a 4K screen).
    float scale = 1.0f;
    int width = 1280, height = 800;
    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
        float sx = 1.0f, sy = 1.0f;
        glfwGetMonitorContentScale(monitor, &sx, &sy);
        scale = sx > 0.0f ? sx : 1.0f;
        if (const GLFWvidmode* mode = glfwGetVideoMode(monitor)) {
            width = static_cast<int>(width * scale);
            height = static_cast<int>(height * scale);
            width = width < mode->width * 9 / 10 ? width : mode->width * 9 / 10;
            height = height < mode->height * 9 / 10 ? height : mode->height * 9 / 10;
        }
    }

    GLFWwindow* window = glfwCreateWindow(width, height, "FoxyHUD Menu - OpenGL 3", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    auto glClearColorFn = reinterpret_cast<PFN_ClearColor>(glfwGetProcAddress("glClearColor"));
    auto glClearFn = reinterpret_cast<PFN_Clear>(glfwGetProcAddress("glClear"));

    demo::InitImGui(scale);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    foxy::AdminPanel panel;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            glfwWaitEventsTimeout(0.1);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        demo::DrawFrame(panel);
        ImGui::Render();

        glClearColorFn(demo::kClearColor[0], demo::kClearColor[1], demo::kClearColor[2], 1.0f);
        glClearFn(kColorBufferBit);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
