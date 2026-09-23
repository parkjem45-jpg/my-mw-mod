#include <jni.h>
#include <thread>
#include <unistd.h>
#include <dlfcn.h>
#include <android/log.h>
#include <cstring>

#include "zygisk.hpp"
#include "hooks.h"
#include "menu.h"

#define LOG_TAG "MW_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static constexpr const char *kGamePackage = "com.Shooter.ModernWarships";
static JNIEnv *g_Env = nullptr;

static void hackThread() {
    void *handle = nullptr;
    while (handle == nullptr) {
        handle = dlopen("libil2cpp.so", RTLD_NOLOAD | RTLD_NOW);
        if (handle == nullptr) {
            usleep(100000);
        }
    }

    uintptr_t libBase = reinterpret_cast<uintptr_t>(handle);
    LOGI("libil2cpp.so loaded at 0x%lx", libBase);
    g_LibBase = libBase;

    InitHooks(libBase);
    Menu::Init();
}

class MWModule : public zygisk::ModuleBase {
public:
    void onLoad([[maybe_unused]] zygisk::Api *api, JNIEnv *env) override {
        g_Env = env;
        LOGI("Zygisk module loaded");
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override {
        if (!g_Env || !args || !args->nice_name) {
            return;
        }

        const char *process = g_Env->GetStringUTFChars(args->nice_name, nullptr);
        if (process && std::strcmp(process, kGamePackage) == 0) {
            LOGI("Target process detected: %s", process);
            std::thread(hackThread).detach();
        }

        if (process) {
            g_Env->ReleaseStringUTFChars(args->nice_name, process);
        }
    }
};

REGISTER_ZYGISK_MODULE(MWModule)
