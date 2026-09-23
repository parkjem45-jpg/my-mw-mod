#include "hooks.h"
#include "offsets.h"
#include "dobby.h"
#include <android/log.h>

#define LOG_TAG "MW_HOOKS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

bool g_GOD_MODE = false;
bool g_INFINITE_AMMO = false;
bool g_INSTANT_RELOAD = false;
uintptr_t g_LibBase = 0;

typedef void (*TakeDamage_t)(void *instance, float damage);
TakeDamage_t orig_TakeDamage = nullptr;

void hook_TakeDamage(void *instance, float damage) {
    if (g_GOD_MODE) return;
    if (orig_TakeDamage) orig_TakeDamage(instance, damage);
}

void patchMemory(uintptr_t address, uint32_t opcode) {
    uint32_t *p = (uint32_t *)address;
    *p = opcode;
}

void InitHooks(uintptr_t libBase) {
    LOGI("Initializing Hooks...");
    void *takeDamageAddr = (void *)(libBase + Offsets::TakeDamage);
    if (DobbyHook(takeDamageAddr, (void *)hook_TakeDamage, (void **)&orig_TakeDamage) == 0) {
        LOGI("Hooked TakeDamage successfully");
    } else {
        LOGI("Failed to hook TakeDamage");
    }
}

void ApplyPatches(uintptr_t libBase) {
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
