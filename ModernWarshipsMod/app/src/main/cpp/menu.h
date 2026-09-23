#pragma once

class Menu {
public:
    static bool isMenuVisible;
    static bool isMinimized;

    static void Init();
    static void Render();
    static void Shutdown();
};
