#include "hooks.h"
#include "offsets.h"
#include "dobby.h"
#include <android/log.h>
#include <cstdio>
#include <cstring>

#define LOG_TAG "MW_HOOKS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

bool g_GOD_MODE = false;
bool g_INFINITE_AMMO = false;
bool g_INSTANT_RELOAD = false;
uintptr_t g_LibBase = 0;

// Control-block state (requested layout: portready 0x338C0,
// featureflags 0x338C4, gamebase 0x338D8, local_player 0x338E0,
// damage_original 0x338E8, cooldown_original 0x338F0,
// ready_original 0x338F8, ammo_original 0x33900).
uint32_t g_PortReady = 0;
uint32_t g_FeatureFlags = 0;
uintptr_t g_GameBase = 0;
void *g_LocalPlayerPtr = nullptr;
uint32_t g_DamageOrig = 0;
uint32_t g_CooldownOrig = 0;
uint32_t g_ReadyOrig = 0;
uint32_t g_AmmoOrig = 0;

// --- Original function pointers ---
// NOTE: signatures are best-guess from prologue/bytes. If the game
// updates, adjust these to the exact managed signatures.
typedef void (*Damage_t)(void *instance, float damage);
typedef float (*Cooldown_t)(void *instance);
typedef void (*Ammo_t)(void *instance);
typedef void *(*LocalPlayer_t)();

static Damage_t orig_Damage = nullptr;
static Cooldown_t orig_Cooldown = nullptr;
static Ammo_t orig_Ammo = nullptr;
static LocalPlayer_t orig_LocalPlayer = nullptr;

// Resolved target addresses (0 = unreadable/unverified, never touch).
static uintptr_t s_damageAddr = 0;
static uintptr_t s_cooldownAddr = 0;
static uintptr_t s_readyAddr = 0;
static uintptr_t s_ammoAddr = 0;

// MATCH state per target. Hooks/patches are only ever applied on MATCH.
static bool s_damageMatch = false;
static bool s_cooldownMatch = false;
static bool s_readyMatch = false;
static bool s_ammoMatch = false;

// Install state (hooks are lazy: installed on toggle-on, removed on toggle-off).
static bool s_damageHooked = false;
static bool s_cooldownHooked = false;
static bool s_readyPatched = false;
static bool s_ammoHooked = false;
static bool s_hookFailLogged = false;

// Ready() is a 2-instruction leaf, so back up both words for restore.
static uint32_t s_readyOrig2 = 0;

// --- Hooks (only installed while the matching cheat is enabled) ---
static void hook_Damage(void *instance, float damage) {
    if (g_GOD_MODE) {
        return; // god mode: swallow damage
    }
    if (orig_Damage) {
        orig_Damage(instance, damage);
    }
}

static float hook_Cooldown(void *instance) {
    if (g_INSTANT_RELOAD) {
        return 0.0f; // no cooldown
    }
    if (orig_Cooldown) {
        return orig_Cooldown(instance);
    }
    return 0.0f;
}

static void hook_Ammo(void *instance) {
    if (g_INFINITE_AMMO) {
        return; // swallow ammo consumption
    }
    if (orig_Ammo) {
        orig_Ammo(instance);
    }
}

// --- Safe memory helpers ---
// Never touch game memory without proving the page is mapped readable first.
// A blind read of a stale RVA is SIGSEGV at inject time.
static bool IsReadable(uintptr_t address) {
    if (address == 0) {
        return false;
    }
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        return false;
    }
    char line[512];
    bool ok = false;
    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start = 0, end = 0;
        char perms[8] = {0};
        if (sscanf(line, "%lx-%lx %7s", &start, &end, perms) == 3) {
            if (address >= start && address < end && perms[0] == 'r') {
                ok = true;
                break;
            }
        }
    }
    fclose(fp);
    return ok;
}

static bool SafeReadU32(uintptr_t address, uint32_t *out) {
    if (!IsReadable(address) || !IsReadable(address + sizeof(uint32_t) - 1)) {
        return false;
    }
    memcpy(out, reinterpret_cast<void *>(address), sizeof(uint32_t));
    return true;
}

static bool VerifyTarget(const char *name, uintptr_t rva, uintptr_t address,
                         uint32_t expected, bool hasExpected, uint32_t *origOut) {
    uint32_t actual = 0;
    if (!SafeReadU32(address, &actual)) {
        LOGI("%s: RVA=0x%lX RESULT=UNREADABLE (skipped, game version may differ)", name,
             (unsigned long)rva);
        return false;
    }
    if (origOut) {
        *origOut = actual;
    }
    if (hasExpected) {
        bool match = (actual == expected);
        LOGI("%s: RVA=0x%lX FIRST=0x%08X EXPECTED=0x%08X RESULT=%s", name, (unsigned long)rva,
             actual, expected, match ? "MATCH" : "MISMATCH");
        return match;
    }
    LOGI("%s: RVA=0x%lX FIRST=0x%08X RESULT=READABLE", name, (unsigned long)rva, actual);
    return true;
}

static void syncFeatureFlags() {
    uint32_t flags = 0;
    if (g_GOD_MODE) flags |= Offsets::FeatureDamage | Offsets::FeatureReady;
    if (g_INFINITE_AMMO) flags |= Offsets::FeatureAmmo;
    if (g_INSTANT_RELOAD) flags |= Offsets::FeatureCooldown | Offsets::FeatureReady;
    g_FeatureFlags = flags;
}

static void LogHookFailOnce() {
    if (!s_hookFailLogged) {
        s_hookFailLogged = true;
        LOGI("Hook install failed (Dobby error). Will keep game unhooked; toggles stay safe.");
    }
}

// On-demand only. NEVER called during InitHooks: invoking an unverified
// address before the IL2CPP runtime is up is an instant SIGSEGV/SIGILL.
void *GetLocalPlayer() {
    if (orig_LocalPlayer && g_LocalPlayerPtr == nullptr) {
        void *p = orig_LocalPlayer();
        if (p) {
            g_LocalPlayerPtr = p;
        }
    }
    return g_LocalPlayerPtr;
}

void InitHooks(uintptr_t libBase) {
    g_LibBase = libBase;
    g_GameBase = libBase;
    LOGI("Initializing hooks... libBase=0x%lX", (unsigned long)libBase);
    if (libBase == 0) {
        LOGI("InitHooks: null libBase, aborting");
        return;
    }

    // Verify first instruction against dump (2026-10-04):
    // Damage/Cooldown 0xD10143FF, Ready 0x3943D000, Ammo 0xA9BE57FE,
    // LocalPlayer 0x394C1000 (readable).
    // Anything UNREADABLE or MISMATCH is left completely alone.
    if (VerifyTarget("Damage", Offsets::Damage, libBase + Offsets::Damage,
                     Offsets::DamageExpected, true, &g_DamageOrig)) {
        s_damageAddr = libBase + Offsets::Damage;
        s_damageMatch = true;
    }
    if (VerifyTarget("Cooldown", Offsets::Cooldown, libBase + Offsets::Cooldown,
                     Offsets::CooldownExpected, true, &g_CooldownOrig)) {
        s_cooldownAddr = libBase + Offsets::Cooldown;
        s_cooldownMatch = true;
    }
    if (VerifyTarget("Ready", Offsets::Ready, libBase + Offsets::Ready, Offsets::ReadyExpected,
                     true, &g_ReadyOrig)) {
        s_readyAddr = libBase + Offsets::Ready;
        s_readyMatch = true;
        // Back up the full 8-byte leaf (LDRB; RET) for later restore.
        uint32_t second = 0;
        if (SafeReadU32(s_readyAddr + 4, &second)) {
            s_readyOrig2 = second;
        } else {
            s_readyMatch = false;
            s_readyAddr = 0;
            LOGI("Ready: second word unreadable, patching disabled");
        }
    }
    if (VerifyTarget("Ammo", Offsets::Ammo, libBase + Offsets::Ammo, Offsets::AmmoExpected, true,
                     &g_AmmoOrig)) {
        s_ammoAddr = libBase + Offsets::Ammo;
        s_ammoMatch = true;
    }
    VerifyTarget("LocalPlayer", Offsets::LocalPlayer, libBase + Offsets::LocalPlayer, 0, false,
                 nullptr);
    if (IsReadable(libBase + Offsets::LocalPlayer)) {
        // Cache the entry point address only. Do NOT call it here.
        orig_LocalPlayer = reinterpret_cast<LocalPlayer_t>(libBase + Offsets::LocalPlayer);
        LOGI("LocalPlayer entry cached (not called yet)");
    } else {
        LOGI("LocalPlayer entry unreadable, helper disabled");
    }

    LOGI("Orig backups: damage=0x%08X cooldown=0x%08X ready=0x%08X ammo=0x%08X", g_DamageOrig,
         g_CooldownOrig, g_ReadyOrig, g_AmmoOrig);

    syncFeatureFlags();

    // Hooks are intentionally NOT installed here. They are installed lazily
    // from ApplyPatches() when a cheat is toggled on, so a wrong or stale
    // signature can never crash a user who never enables cheats.
    g_PortReady = 1;
    LOGI("Init done. portready=1 hooks deferred until toggle");
}

void ApplyPatches(uintptr_t libBase) {
    if (libBase == 0 || libBase != g_LibBase) {
        return;
    }

    syncFeatureFlags();

    // --- Damage (god mode): lazy Dobby hook ---
    bool wantDamage = g_GOD_MODE && s_damageMatch && s_damageAddr != 0;
    if (wantDamage && !s_damageHooked) {
        if (DobbyHook(reinterpret_cast<void *>(s_damageAddr),
                      reinterpret_cast<void *>(hook_Damage),
                      reinterpret_cast<void **>(&orig_Damage)) == 0) {
            s_damageHooked = true;
            LOGI("Damage hook installed @0x%lX", (unsigned long)Offsets::Damage);
        } else {
            LogHookFailOnce();
        }
    } else if (!wantDamage && s_damageHooked) {
        DobbyDestroy(reinterpret_cast<void *>(s_damageAddr));
        orig_Damage = nullptr;
        s_damageHooked = false;
        LOGI("Damage hook removed");
    }

    // --- Cooldown (instant reload): lazy Dobby hook ---
    bool wantCooldown = g_INSTANT_RELOAD && s_cooldownMatch && s_cooldownAddr != 0;
    if (wantCooldown && !s_cooldownHooked) {
        if (DobbyHook(reinterpret_cast<void *>(s_cooldownAddr),
                      reinterpret_cast<void *>(hook_Cooldown),
                      reinterpret_cast<void **>(&orig_Cooldown)) == 0) {
            s_cooldownHooked = true;
            LOGI("Cooldown hook installed @0x%lX", (unsigned long)Offsets::Cooldown);
        } else {
            LogHookFailOnce();
        }
    } else if (!wantCooldown && s_cooldownHooked) {
        DobbyDestroy(reinterpret_cast<void *>(s_cooldownAddr));
        orig_Cooldown = nullptr;
        s_cooldownHooked = false;
        LOGI("Cooldown hook removed");
    }

    // --- Ready (always ready): byte-patch, NOT a Dobby hook ---
    // Ready() is a 2-instruction leaf; an inline hooker may need more room
    // than the function has, so patch MOVZ W0,#1 ; RET via DobbyCodePatch
    // (handles page protection + cache flush) and restore on toggle-off.
    bool wantReady = (g_INSTANT_RELOAD || g_GOD_MODE) && s_readyMatch && s_readyAddr != 0;
    if (wantReady && !s_readyPatched) {
        uint32_t patch[2] = {0x52800020u, 0xD65F03C0u}; // MOVZ W0, #1; RET
        if (DobbyCodePatch(reinterpret_cast<void *>(s_readyAddr), reinterpret_cast<uint8_t *>(patch),
                           sizeof(patch)) == 0) {
            s_readyPatched = true;
            LOGI("Ready patched @0x%lX", (unsigned long)Offsets::Ready);
        } else {
            LogHookFailOnce();
        }
    } else if (!wantReady && s_readyPatched) {
        uint32_t restore[2] = {g_ReadyOrig, s_readyOrig2};
        DobbyCodePatch(reinterpret_cast<void *>(s_readyAddr), reinterpret_cast<uint8_t *>(restore),
                       sizeof(restore));
        s_readyPatched = false;
        LOGI("Ready patch restored");
    }

    // --- Ammo (infinite ammo): lazy Dobby hook ---
    bool wantAmmo = g_INFINITE_AMMO && s_ammoMatch && s_ammoAddr != 0;
    if (wantAmmo && !s_ammoHooked) {
        if (DobbyHook(reinterpret_cast<void *>(s_ammoAddr), reinterpret_cast<void *>(hook_Ammo),
                      reinterpret_cast<void **>(&orig_Ammo)) == 0) {
            s_ammoHooked = true;
            LOGI("Ammo hook installed @0x%lX", (unsigned long)Offsets::Ammo);
        } else {
            LogHookFailOnce();
        }
    } else if (!wantAmmo && s_ammoHooked) {
        DobbyDestroy(reinterpret_cast<void *>(s_ammoAddr));
        orig_Ammo = nullptr;
        s_ammoHooked = false;
        LOGI("Ammo hook removed");
    }
}
