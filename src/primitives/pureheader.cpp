// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "primitives/pureheader.h"
#include "chainparams.h"
#include "crypto/gr_hash/gr_hash.h"
#include "hash.h"
#include "utilstrencodings.h"
void CPureBlockHeader::SetBaseVersion(int32_t nBaseVersion, int32_t nChainId)
{
    assert(nBaseVersion >= 1 && nBaseVersion < VERSION_AUXPOW);
    assert(!IsAuxpow());
    nVersion = nBaseVersion | (nChainId * VERSION_CHAIN_START);
}
uint256 CPureBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}
uint256 CPureBlockHeader::GetPoWHash() const
{
    const uint8_t* pbegin = (const uint8_t*)BEGIN(nVersion);
    return GhostriderHash(pbegin, 80);
}

// Height-aware PoW hash — switches to RandomXv2 at nRandomXV2Height
#include "crypto/randomx_rabid/randomx_rabid.h"

uint256 CPureBlockHeader::GetPoWHash(uint32_t nHeight) const
{
    const Consensus::Params& params = Params().GetConsensus(nHeight);
    if (params.nRandomXV2Height > 0 && nHeight >= params.nRandomXV2Height) {
        const uint8_t* pbegin = (const uint8_t*)BEGIN(nVersion);
        return RandomXV2Hash(nHeight, pbegin, 80);
    }
    // Pre-fork: GhostRider
    const uint8_t* pbegin = (const uint8_t*)BEGIN(nVersion);
    return GhostriderHash(pbegin, 80);
}
