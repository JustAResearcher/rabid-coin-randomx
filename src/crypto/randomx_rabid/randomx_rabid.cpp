// Copyright (c) 2024 The RabidCoin Core developers
// Distributed under the MIT software license.
//
// RandomXv2 wrapper for RabidCoin daemon.
// Uses the RandomX library already present in src/crypto/randomx/
// Key rotates every KEY_PERIOD blocks (same epoch logic as Monero).

#include "randomx_rabid.h"
#include "crypto/randomx/randomx.h"

#ifdef WIN32
#include <windows.h>
static CRITICAL_SECTION s_cs;
static bool s_cs_init = false;
struct WinMutex {
    WinMutex() { if(!s_cs_init){InitializeCriticalSection(&s_cs);s_cs_init=true;} }
};
static WinMutex s_mutex_init;
struct WinLockGuard {
    WinLockGuard() { EnterCriticalSection(&s_cs); }
    ~WinLockGuard() { LeaveCriticalSection(&s_cs); }
};
#define MUTEX_TYPE int
#define LOCK_GUARD WinLockGuard
#else
#include <mutex>
#define MUTEX_TYPE std::mutex
#define LOCK_GUARD std::lock_guard<std::mutex>
#endif
#include <stdexcept>
#include <cstring>

// Seed key epoch — same as Monero: every 2048 blocks
static constexpr uint32_t KEY_PERIOD = 2048;

static uint32_t         s_currentEpoch = UINT32_MAX;
static randomx_cache*   s_cache        = nullptr;
static randomx_dataset* s_dataset      = nullptr;
static randomx_vm*      s_vm           = nullptr;
#ifndef WIN32
static std::mutex       s_mutex;
#endif

static uint32_t KeyEpoch(uint32_t height)
{
    return (height / KEY_PERIOD) * KEY_PERIOD;
}

// 4-byte LE height as seed key — simple and deterministic
static void MakeSeedKey(uint32_t epoch, uint8_t key[4])
{
    key[0] = (epoch      ) & 0xFF;
    key[1] = (epoch >>  8) & 0xFF;
    key[2] = (epoch >> 16) & 0xFF;
    key[3] = (epoch >> 24) & 0xFF;
}

void RandomXV2_InitCache(uint32_t nHeight)
{
    uint32_t epoch = KeyEpoch(nHeight);
    #ifdef WIN32
    WinLockGuard _win_lock;
#else
    std::lock_guard<std::mutex> lock(s_mutex);
#endif

    if (epoch == s_currentEpoch && s_vm != nullptr)
        return;

    // Tear down existing state
    if (s_vm)      { randomx_destroy_vm(s_vm);           s_vm      = nullptr; }
    if (s_dataset) { randomx_release_dataset(s_dataset); s_dataset = nullptr; }
    if (s_cache)   { randomx_release_cache(s_cache);     s_cache   = nullptr; }

    uint8_t key[4];
    MakeSeedKey(epoch, key);

    // Always allocate cache
    randomx_flags flags = RANDOMX_FLAG_DEFAULT;
    s_cache = randomx_alloc_cache(flags);
    if (!s_cache)
        throw std::runtime_error("RandomXV2: randomx_alloc_cache failed");
    randomx_init_cache(s_cache, key, sizeof(key));

    // Try full dataset (fast mode) — fall back to light mode if alloc fails
    s_dataset = randomx_alloc_dataset(RANDOMX_FLAG_DEFAULT);
    if (s_dataset) {
        randomx_init_dataset(s_dataset, s_cache, 0, randomx_dataset_item_count());
        s_vm = randomx_create_vm(RANDOMX_FLAG_FULL_MEM, s_cache, s_dataset);
    }

    if (!s_vm) {
        // Light mode fallback
        if (s_dataset) { randomx_release_dataset(s_dataset); s_dataset = nullptr; }
        s_vm = randomx_create_vm(RANDOMX_FLAG_DEFAULT, s_cache, nullptr);
    }

    if (!s_vm)
        throw std::runtime_error("RandomXV2: randomx_create_vm failed");

    s_currentEpoch = epoch;
}

uint256 RandomXV2Hash(uint32_t nHeight, const uint8_t* input, size_t len)
{
    RandomXV2_InitCache(nHeight);

    uint256 result;
    #ifdef WIN32
    WinLockGuard _win_lock;
#else
    std::lock_guard<std::mutex> lock(s_mutex);
#endif
    randomx_calculate_hash(s_vm, input, len, result.begin());
    return result;
}
