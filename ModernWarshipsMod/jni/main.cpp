#include <jni.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <dlfcn.h>
#include <android/log.h>
#include <cstring>

#include "zygisk.hpp"
#include "menu.h"
#include "hooks.h"

#define LOG_TAG "MW_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

using namespace zygisk;

const char *game_package_name = "com.Shooter.ModernWarships";

void hack_thread() {
    void *handle = nullptr;
    while (!handle) {
        handle = dlopen("libil2cpp.so", RTLD_NOLOAD | RTLD_NOW);
        usleep(100000);
    }

    uintptr_t libBase = (uintptr_t)handle;
    LOGI("libil2cpp.so found at base: 0x%lX", libBase);
    g_LibBase = libBase;

    InitHooks(libBase);
    Menu::Init();
}

class MWModule : public ModuleBase {
public:
    void onLoad() override {
        LOGI("Module loaded");
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        const char *process = args->nice_name;
        if (process && strcmp(process, game_package_name) == 0) {
            LOGI("Target app detected: %s", process);
        }
    }

    void postAppSpecialize(const AppSpecializeArgs *args) override {
        const char *process = args->nice_name;
        if (process && strcmp(process, game_package_name) == 0) {
            LOGI("Starting hack thread for %s", process);
            std::thread(hack_thread).detach();
        }
    }
};

REGISTER_ZYGISK_MODULE(MWModule)
