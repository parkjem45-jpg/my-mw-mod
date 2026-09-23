#include <jni.h>
#include <thread>
#include <unistd.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdint>
#include <android/log.h>
#include <cstring>

#include "zygisk.hpp"
#include "hooks.h"
#include "menu.h"
#include "egl_hook.h"

#define LOG_TAG "MW_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static constexpr const char *kGamePackage = "com.Shooter.ModernWarships";
static JNIEnv *g_Env = nullptr;

static uintptr_t getModuleBase(const char *libName) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[1024];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, libName)) {
            // line format: start-end perms ...
            char *dash = strchr(line, '-');
            if (dash) {
                *dash = '\0';
                unsigned long addr = 0;
                sscanf(line, "%lx", &addr);
                base = static_cast<uintptr_t>(addr);
                break;
            }
        }
    }
    fclose(fp);
    return base;
}

static void hackThread() {
    // Wait for libil2cpp.so to load
    void *handle = nullptr;
    while (handle == nullptr) {
        handle = dlopen("libil2cpp.so", RTLD_NOLOAD | RTLD_NOW);
        if (handle == nullptr) {
            usleep(100000);
        }
    }
    dlclose(handle);

    // Get real base address from /proc/self/maps (dlopen handle is NOT the base)
    uintptr_t libBase = 0;
    while (libBase == 0) {
        libBase = getModuleBase("libil2cpp.so");
        if (libBase == 0) {
            usleep(100000);
        }
    }
    LOGI("libil2cpp.so loaded at 0x%lx", (unsigned long)libBase);
    g_LibBase = libBase;

    InitHooks(libBase);
    InitEGLHook();
    // NOTE: Menu::Init() is intentionally NOT called here: ImGui needs a
    // current GL context, so egl_hook.cpp runs it on the first
    // eglSwapBuffers frame instead.
}

class MWModule : public zygisk::ModuleBase {
public:
    void onLoad([[maybe_unused]] zygisk::Api *api, JNIEnv *env) override {
        g_Env = env;
        LOGI("MW_MOD: onLoad() called!");
    }

    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override {
        if (!g_Env || !args) {
            return;
        }
        const char *process = nullptr;
        if (args->nice_name) {
            process = g_Env->GetStringUTFChars(args->nice_name, nullptr);
        }
        if (process) {
            LOGI("MW_MOD: preAppSpecialize called for process: %s", process);
            g_Env->ReleaseStringUTFChars(args->nice_name, process);
        } else {
            LOGI("MW_MOD: preAppSpecialize called (no process name)");
        }
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        if (!g_Env || !args) {
            LOGI("MW_MOD: postAppSpecialize called (no args/env)");
            return;
        }
        if (!args->nice_name) {
            LOGI("MW_MOD: postAppSpecialize called (no process name)");
            return;
        }

        const char *process = g_Env->GetStringUTFChars(args->nice_name, nullptr);
        if (process) {
            LOGI("MW_MOD: postAppSpecialize called for process: %s", process);
        }
        if (process && std::strcmp(process, kGamePackage) == 0) {
            LOGI("MW_MOD: target process detected: %s, starting hack thread", process);
            std::thread(hackThread).detach();
        }

        if (process) {
            g_Env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }
};

REGISTER_ZYGISK_MODULE(MWModule)
