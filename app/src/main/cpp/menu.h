#pragma once

class Menu {
public:
    static bool isMenuVisible;
    static bool isMinimized;

    static void Init();
    static void Render();
    static void Shutdown();
    // Called every frame by the EGL hook with the real surface size, since
    // the Android backend is initialized without an ANativeWindow.
    static void SetDisplaySize(float w, float h);
};
