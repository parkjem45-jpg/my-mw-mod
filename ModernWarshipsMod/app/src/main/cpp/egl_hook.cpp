#include "egl_hook.h"
#include "menu.h"
#include "dobby.h"

#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <android/log.h>
#include <EGL/egl.h>

#define LOG_TAG "MW_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

typedef EGLBoolean (*eglSwapBuffers_t)(EGLDisplay display, EGLSurface surface);

static eglSwapBuffers_t orig_eglSwapBuffers = nullptr;
static bool g_ImGuiReady = false;

// Log which graphics libraries are mapped into this process so Logcat shows
// whether the game renders via EGL/GLES or Vulkan.
static void LogGraphicsModules() {
    bool hasEgl = false, hasGles2 = false, hasGles3 = false, hasVulkan = false;
    FILE *fp = fopen("/proc/self/maps", "r");
    if (fp) {
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "libEGL.so")) hasEgl = true;
            if (strstr(line, "libGLESv2.so")) hasGles2 = true;
            if (strstr(line, "libGLESv3.so")) hasGles3 = true;
            if (strstr(line, "libvulkan.so")) hasVulkan = true;
        }
        fclose(fp);
    }
    LOGI("MW_MOD: gfx modules: EGL=%d GLESv2=%d GLESv3=%d Vulkan=%d", hasEgl, hasGles2, hasGles3,
         hasVulkan);
    if (hasVulkan && !hasEgl) {
        LOGI("MW_MOD: WARNING game looks Vulkan-only, EGL menu may never draw");
    }
}

static EGLBoolean hk_eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    if (!g_ImGuiReady) {
        // First frame: a GL context is current here, safe to init ImGui now.
        EGLint w = 0, h = 0;
        eglQuerySurface(display, surface, EGL_WIDTH, &w);
        eglQuerySurface(display, surface, EGL_HEIGHT, &h);
        LOGI("MW_MOD: first eglSwapBuffers, surface=%dx%d", w, h);
        Menu::Init();
        if (w > 0 && h > 0) {
            Menu::SetDisplaySize((float)w, (float)h);
        }
        g_ImGuiReady = true;
        LOGI("MW_MOD: ImGui init done");
    } else {
        // Keep display size in sync (rotation / resize).
        EGLint w = 0, h = 0;
        eglQuerySurface(display, surface, EGL_WIDTH, &w);
        eglQuerySurface(display, surface, EGL_HEIGHT, &h);
        if (w > 0 && h > 0) {
            Menu::SetDisplaySize((float)w, (float)h);
        }
    }

    if (g_ImGuiReady) {
        Menu::Render();
    }

    return orig_eglSwapBuffers(display, surface);
}

void InitEGLHook() {
    LogGraphicsModules();

    void *handle = dlopen("libEGL.so", RTLD_NOW);
    if (!handle) {
        LOGI("MW_MOD: dlopen libEGL.so failed, trying libGLESv2.so");
        handle = dlopen("libGLESv2.so", RTLD_NOW);
    }
    if (!handle) {
        LOGI("MW_MOD: no EGL/GLES handle, EGL hook skipped");
        return;
    }

    void *sym = dlsym(handle, "eglSwapBuffers");
    if (!sym) {
        LOGI("MW_MOD: dlsym eglSwapBuffers failed");
        return;
    }

    if (DobbyHook(sym, (void *)hk_eglSwapBuffers, (void **)&orig_eglSwapBuffers) == 0) {
        LOGI("MW_MOD: eglSwapBuffers hook installed");
    } else {
        LOGI("MW_MOD: eglSwapBuffers hook FAILED");
    }
}
