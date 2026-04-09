// Copyright (c) 2024 The RabidCoin Core developers
// Distributed under the MIT software license.
//
// RandomXv2 wrapper — activates at nRandomXV2Height (block 8000)
// MoneroV2 config: ProgramSize=384, V2 tweaks enabled

#ifndef RABIDCOIN_RANDOMX_RABID_H
#define RABIDCOIN_RANDOMX_RABID_H

#include <stdint.h>
#include "uint256.h"

// Hash a block header using RandomXv2 config.
// nHeight: the block height being hashed (used to derive seed key epoch)
// input:   80-byte block header
uint256 RandomXV2Hash(uint32_t nHeight, const uint8_t* input, size_t len);

// Initialise (or re-initialise) the RandomXv2 VM for the given block height.
// Safe to call multiple times — no-op if already initialised for this key epoch.
void RandomXV2_InitCache(uint32_t nHeight);

#endif // RABIDCOIN_RANDOMX_RABID_H
