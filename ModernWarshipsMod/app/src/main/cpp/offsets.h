#pragma once

#include <cstdint>

namespace Offsets {
    // Target RVAs in libil2cpp.so (verified 2026-10-04, see bytes below)
    // Damage        RVA 0x3BD26E0  FILE 0x3BCE6E0  FIRST 0xD10143FF (FF 43 01 D1 ...)
    // Cooldown      RVA 0x388FE64  FILE 0x388BE64  FIRST 0xD10143FF (FF 43 01 D1 ...)
    // Ready         RVA 0x3B23018  FILE 0x3B1F018  FIRST 0x3943D000 (00 D0 43 39 ...)
    // Ammo          RVA 0x3B2256C  FILE 0x3B1E56C  FIRST 0xA9BE57FE (FE 57 BE A9 ...)
    // LocalPlayer   RVA 0x33A2E18  FILE 0x339EE18  FIRST 0x394C1000 (readable helper)
    constexpr uintptr_t Damage      = 0x3BD26E0;
    constexpr uintptr_t Cooldown    = 0x388FE64;
    constexpr uintptr_t Ready       = 0x3B23018;
    constexpr uintptr_t Ammo        = 0x3B2256C;
    constexpr uintptr_t LocalPlayer = 0x33A2E18;

    // Expected first instruction (little-endian u32) for verification
    constexpr uint32_t DamageExpected   = 0xD10143FF;
    constexpr uint32_t CooldownExpected = 0xD10143FF;
    constexpr uint32_t ReadyExpected    = 0x3943D000;
    constexpr uint32_t AmmoExpected     = 0xA9BE57FE;

    // Control-block layout as provided (offsets into helper region).
    // NOTE: kept as constants for external tooling; runtime state lives in
    // hooks.cpp statics (we do NOT blindly write to libBase + these, that
    // region is executable code, not scratch space).
    constexpr uintptr_t PortReadyOff      = 0x338C0; // u32 portready
    constexpr uintptr_t FeatureFlagsOff   = 0x338C4; // u32 featureflags
    constexpr uintptr_t GameBaseOff        = 0x338D8; // u64 gamebase
    constexpr uintptr_t LocalPlayerPtrOff = 0x338E0; // u64 local_player
    constexpr uintptr_t DamageOrigOff     = 0x338E8; // u32 damage_original
    constexpr uintptr_t CooldownOrigOff   = 0x338F0; // u32 cooldown_original
    constexpr uintptr_t ReadyOrigOff      = 0x338F8; // u32 ready_original
    constexpr uintptr_t AmmoOrigOff       = 0x33900; // u32 ammo_original

    // Feature flag bits (synced from g_GOD_MODE / g_INFINITE_AMMO / g_INSTANT_RELOAD)
    constexpr uint32_t FeatureDamage   = 1u << 0; // god mode (Damage hook)
    constexpr uint32_t FeatureCooldown = 1u << 1; // instant reload (Cooldown hook)
    constexpr uint32_t FeatureReady    = 1u << 2; // always ready (Ready hook)
    constexpr uint32_t FeatureAmmo     = 1u << 3; // infinite ammo (Ammo hook)
}
