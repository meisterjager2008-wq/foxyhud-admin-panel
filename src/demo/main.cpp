// Standalone demo: opens a window, fakes a game frame in the background and
// draws the admin panel on top. Press INSERT to toggle the panel.

#include <cstdio>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "foxyhud/admin_panel.h"
#include "foxyhud/theme.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "mock_backend.h"

namespace {

void GlfwError(int code, const char* description) { std::fprintf(stderr, "GLFW error %d: %s\n", code, description); }

// Stand-in for your game's frame, so the panel has something behind it.
void DrawFakeGameScene() {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 a = vp->Pos;
    const ImVec2 b(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y);
    dl->AddRectFilledMultiColor(a, b, IM_COL32(38, 48, 68, 255), IM_COL32(38, 48, 68, 255), IM_COL32(12, 14, 20, 255),
                                IM_COL32(12, 14, 20, 255));

    const float cx = a.x + vp->Size.x * 0.5f;
    const float horizon = a.y + vp->Size.y * 0.62f;
    for (int i = 0; i <= 24; ++i) {
        const float x = a.x + vp->Size.x * i / 24.0f;
        dl->AddLine(ImVec2(x, horizon), ImVec2(cx + (x - cx) * 3.0f, b.y), IM_COL32(90, 130, 210, 26));
    }
    for (int i = 1; i <= 10; ++i) {
        const float t = i / 10.0f;
        const float y = horizon + (b.y - horizon) * t * t;
        dl->AddLine(ImVec2(a.x, y), ImVec2(b.x, y), IM_COL32(90, 130, 210, 20));
    }

    const char* hint = "Your game renders here  -  press INSERT to toggle the admin panel";
    const ImVec2 ts = ImGui::CalcTextSize(hint);
    dl->AddText(ImVec2(cx - ts.x * 0.5f, b.y - ts.y - 16.0f), IM_COL32(255, 255, 255, 90), hint);
}

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

    GLFWwindow* window = glfwCreateWindow(width, height, "FoxyHUD Admin Panel - Demo", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = "foxyhud_demo.ini";

    // 1) Fonts + style, 2) platform/renderer backends (your game uses its own, e.g. DX11).
    foxy::theme::LoadFonts(scale);
    foxy::theme::Apply(scale);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    MockBackend backend;
    foxy::AdminPanel panel(backend);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            glfwWaitEventsTimeout(0.1);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        backend.Update(ImGui::GetIO().DeltaTime);
        DrawFakeGameScene();
        panel.Render();

        ImGui::Render();
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
