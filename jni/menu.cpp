#include "menu.h"
#include "hooks.h"
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_android.h"

bool Menu::isMenuVisible = true;
bool Menu::isMinimized = false;

static bool show_aimbot_tab = false;

void Menu::Init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;

    ImGui_ImplAndroid_Init(nullptr);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.90f, 0.30f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.00f, 0.40f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.30f, 0.10f, 1.00f);
}

void Menu::Render() {
    if (!isMenuVisible) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();

    if (isMinimized) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Once);
        ImGui::Begin("Mini", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);
        if (ImGui::Button("Open Menu")) isMinimized = false;
        ImGui::End();
    } else {
        ImGui::SetNextWindowSize(ImVec2(450, 500), ImGuiCond_Once);
        ImGui::Begin("MW Mod Menu", nullptr, ImGuiWindowFlags_NoCollapse);

        if (ImGui::Button("Minimize")) isMinimized = true;
        ImGui::SameLine();
        if (ImGui::Button("Hide Menu")) isMenuVisible = false;
        ImGui::Separator();

        if (ImGui::BeginTabBar("Tabs")) {
            if (ImGui::BeginTabItem("Aimbot")) {
                ImGui::Checkbox("Enable Aimbot", &show_aimbot_tab);
                ImGui::Checkbox("Ignore invisible targets", &show_aimbot_tab);
                ImGui::Checkbox("Ignore knocked targets", &show_aimbot_tab);
                ImGui::Checkbox("Force lock", &show_aimbot_tab);
                const char* hitboxItems[] = { "Head", "Body", "Neck" };
                static int currentHitbox = 0;
                ImGui::Combo("Hitbox", &currentHitbox, hitboxItems, IM_ARRAYSIZE(hitboxItems));
                ImGui::EndTabItem();
            }
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
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Menu::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
}
