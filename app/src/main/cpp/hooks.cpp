#include "hooks.h"
#include "offsets.h"
#include "dobby.h"
#include <android/log.h>
#include <cstring>

#define LOG_TAG "MW_HOOKS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

bool g_GOD_MODE = false;
bool g_INFINITE_AMMO = false;
bool g_INSTANT_RELOAD = false;
uintptr_t g_LibBase = 0;

typedef void (*TakeDamage_t)(void *instance, float damage);
static TakeDamage_t orig_TakeDamage = nullptr;

static void hook_TakeDamage(void *instance, float damage) {
    if (g_GOD_MODE) {
        return;
    }
    if (orig_TakeDamage) {
        orig_TakeDamage(instance, damage);
    }
}

static void patchMemory(uintptr_t address, uint32_t opcode) {
    if (address == 0) {
        return;
    }
    auto *ptr = reinterpret_cast<uint32_t *>(address);
    *ptr = opcode;
}

void InitHooks(uintptr_t libBase) {
    g_LibBase = libBase;
    LOGI("Initializing hooks...");
    void *takeDamageAddr = reinterpret_cast<void *>(libBase + Offsets::TakeDamage);
    if (DobbyHook(takeDamageAddr, reinterpret_cast<void *>(hook_TakeDamage), reinterpret_cast<void **>(&orig_TakeDamage)) == 0) {
        LOGI("TakeDamage hook installed");
    } else {
        LOGI("Failed to install TakeDamage hook");
    }
}

void ApplyPatches(uintptr_t libBase) {
    if (libBase == 0) {
        return;
    }

    if (g_GOD_MODE) {
        patchMemory(libBase + Offsets::IsDead_MOV, 0x52800000);
        patchMemory(libBase + Offsets::IsDead_RET, 0xD65F03C0);
    }

    if (g_INFINITE_AMMO) {
        patchMemory(libBase + Offsets::ClipSize_MOV, 0x52807CF0);
        patchMemory(libBase + Offsets::ClipSize_RET, 0xD65F03C0);
        patchMemory(libBase + Offsets::StorageType_MOV, 0x52800000);
        patchMemory(libBase + Offsets::StorageType_RET, 0xD65F03C0);
    }

    if (g_INSTANT_RELOAD) {
        patchMemory(libBase + Offsets::ReloadTime_MOV, 0x52800000);
        patchMemory(libBase + Offsets::ReloadTime_RET, 0xD65F03C0);
    }
}
