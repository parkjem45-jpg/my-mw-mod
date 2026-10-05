#include "menu.h"
#include "hooks.h"

#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <time.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_android.h"

bool Menu::isMenuVisible = true;
bool Menu::isMinimized = false;

static bool showPlayerTab = false;

void Menu::Init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Sane default until the EGL hook reports the real surface size.
    // (SetDisplaySize() keeps this in sync every frame after that.)
    io.DisplaySize = ImVec2(1280.0f, 720.0f);

    // NOTE: the Android backend is initialized with a null ANativeWindow
    // because Zygisk has none. Its NewFrame() would call
    // ANativeWindow_getWidth(nullptr) and SIGSEGV, so Render() never calls
    // it -- display size and delta time are maintained manually below.
    ImGui_ImplAndroid_Init(nullptr);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(12.0f, 12.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(10.0f, 6.0f);
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 6.0f;

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.90f, 0.30f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.40f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.25f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.30f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.90f, 0.30f, 0.10f, 0.60f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
}

void Menu::Render() {
    if (!isMenuVisible) {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    // Do NOT call ImGui_ImplAndroid_NewFrame(): with a null ANativeWindow it
    // dereferences nullptr and crashes. Maintain io state manually instead.
    {
        ImGuiIO &io = ImGui::GetIO();
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        double now = (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
        static double last = 0.0;
        io.DeltaTime = last > 0.0 ? (float)(now - last) : (1.0f / 60.0f);
        if (io.DeltaTime <= 0.0f) {
            io.DeltaTime = 1.0f / 60.0f;
        }
        last = now;
        // DisplaySize is owned by SetDisplaySize(); never let it stay 0.
        if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f) {
            io.DisplaySize = ImVec2(1280.0f, 720.0f);
        }
    }
    ImGui::NewFrame();

    if (isMinimized) {
        ImGui::SetNextWindowPos(ImVec2(25.0f, 25.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSizeConstraints(ImVec2(120.0f, 0.0f), ImVec2(300.0f, 1000.0f));
        ImGui::Begin("Toggle", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Open Menu")) {
            isMinimized = false;
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(420.0f, 360.0f), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2(30.0f, 30.0f), ImGuiCond_Once);
    ImGui::Begin("Modern Warships Menu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Minimize")) {
        isMinimized = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Close")) {
        isMenuVisible = false;
    }

    ImGui::Separator();

    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Player")) {
            ImGui::Checkbox("God Mode", &g_GOD_MODE);
            ImGui::Checkbox("Infinite Ammo", &g_INFINITE_AMMO);
            ImGui::Checkbox("Instant Reload", &g_INSTANT_RELOAD);
            ApplyPatches(g_LibBase);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Menu::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
}

void Menu::SetDisplaySize(float w, float h) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(w, h);
}
