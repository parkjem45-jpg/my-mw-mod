#pragma once
#include <cstdint>

extern bool g_GOD_MODE;
extern bool g_INFINITE_AMMO;
extern bool g_INSTANT_RELOAD;
extern uintptr_t g_LibBase;

void InitHooks(uintptr_t libBase);
void ApplyPatches(uintptr_t libBase);
