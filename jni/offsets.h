#pragma once
#include <cstdint>

namespace Offsets {
    // God Mode
    constexpr uintptr_t TakeDamage = 0x2B60E58;
    constexpr uintptr_t IsDead_MOV = 0x2B60CA0;
    constexpr uintptr_t IsDead_RET = 0x2B60CA4;

    // Infinite Ammo
    constexpr uintptr_t ClipSize_MOV   = 0x3068920;
    constexpr uintptr_t ClipSize_RET   = 0x3068924;
    constexpr uintptr_t StorageType_MOV = 0x34064A8;
    constexpr uintptr_t StorageType_RET = 0x34064AC;

    // Instant Reload
    constexpr uintptr_t ReloadTime_MOV = 0x3068A28;
    constexpr uintptr_t ReloadTime_RET = 0x3068A2C;
}
