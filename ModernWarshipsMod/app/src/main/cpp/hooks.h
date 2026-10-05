#pragma once

#include <cstdint>

// Menu toggles (bound to ImGui checkboxes in menu.cpp)
extern bool g_GOD_MODE;
extern bool g_INFINITE_AMMO;
extern bool g_INSTANT_RELOAD;
extern uintptr_t g_LibBase;

// Control-block state (mirrors provided layout: portready 0x338C0,
// featureflags 0x338C4, gamebase 0x338D8, local_player 0x338E0,
// damage_original 0x338E8, cooldown_original 0x338F0,
// ready_original 0x338F8, ammo_original 0x33900).
extern uint32_t g_PortReady;
extern uint32_t g_FeatureFlags;
extern uintptr_t g_GameBase;
extern void *g_LocalPlayerPtr;
extern uint32_t g_DamageOrig;
extern uint32_t g_CooldownOrig;
extern uint32_t g_ReadyOrig;
extern uint32_t g_AmmoOrig;

void InitHooks(uintptr_t libBase);
void ApplyPatches(uintptr_t libBase);
void *GetLocalPlayer();
