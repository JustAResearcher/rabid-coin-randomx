// Copyright (c) 2015-2022 The Rabidcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "amount.h"
#include "chain.h"
#include "chainparams.h"

bool AllowDigishieldMinDifficultyForBlock(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params);
CAmount GetRabidcoinBlockSubsidy(int nHeight, const Consensus::Params& consensusParams, uint256 prevHash);
unsigned int CalculateRabidcoinNextWorkRequired(const CBlockIndex* pindexLast, int64_t nLastRetargetTime, const Consensus::Params& params);

/**
 * Check proof-of-work of a block header, taking auxpow into account.
 * @param block   The block header.
 * @param params  Consensus parameters.
 * @param nHeight Block height (used to pick GhostRider vs RandomXv2). Pass
 *                -1 (default) for pre-context calls. With -1, the non-auxpow
 *                hash check is skipped and must be redone by a contextual
 *                caller (ContextualCheckBlockHeader). Auxpow checks still
 *                run regardless since they don't depend on this chain's algo.
 * @return True iff the PoW is correct.
 */
bool CheckAuxPowProofOfWork(const CBlockHeader& block, const Consensus::Params& params, int nHeight = -1);


