// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>

unsigned int DarkGravityWave(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params) {
    /* current difficulty formula, dash - DarkGravity v3, written by Evan Duffield - evan@dash.org */
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    // int64_t nPastBlocks = 24;
    int64_t nPastBlocks = params.lwmaAveragingWindow;;

    // make sure we have at least (nPastBlocks + 1) blocks, otherwise just return powLimit
    if (!pindexLast || pindexLast->nHeight < nPastBlocks) {
        return bnPowLimit.GetCompact();
    }

    if (params.fPowAllowMinDifficultyBlocks) {
        // recent block is more than 2 hours old
        if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + 2 * 60 * 60) {
            return bnPowLimit.GetCompact();
        }
        // recent block is more than 10 minutes old
        if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing * 4) {
            arith_uint256 bnNew = arith_uint256().SetCompact(pindexLast->nBits) * 10;
            if (bnNew > bnPowLimit) {
                bnNew = bnPowLimit;
            }
            return bnNew.GetCompact();
        }
    }

    const CBlockIndex *pindex = pindexLast;
    arith_uint256 bnPastTargetAvg;

    for (unsigned int nCountBlocks = 1; nCountBlocks <= nPastBlocks; nCountBlocks++) {
        arith_uint256 bnTarget = arith_uint256().SetCompact(pindex->nBits);
        if (nCountBlocks == 1) {
            bnPastTargetAvg = bnTarget;
        } else {
            // NOTE: that's not an average really...
            bnPastTargetAvg = (bnPastTargetAvg * nCountBlocks + bnTarget) / (nCountBlocks + 1);
        }

        if(nCountBlocks != nPastBlocks) {
            assert(pindex->pprev); // should never fail
            pindex = pindex->pprev;
        }
    }

    arith_uint256 bnNew(bnPastTargetAvg);

    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindex->GetBlockTime();
    // NOTE: is this accurate? nActualTimespan counts it for (nPastBlocks - 1) blocks only...
    int64_t nTargetTimespan = nPastBlocks * params.nPowTargetSpacing;

    if (nActualTimespan < nTargetTimespan/3)
        nActualTimespan = nTargetTimespan/3;
    if (nActualTimespan > nTargetTimespan*3)
        nActualTimespan = nTargetTimespan*3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > bnPowLimit) {
        bnNew = bnPowLimit;
    }

    return bnNew.GetCompact();
}

// LWMA-3 for BTC/Zcash clones
// Copyright (c) 2017-2018 The Bitcoin Gold developers
// MIT License
// Algorithm by Zawy, a modification of WT-144 by Tom Harding
// Code by h4x3rotab of BTC Gold, modified/updated by Zawy
// Updated to LWMA-3 by iamstenman (MicroBitcoin)
// For change/updates, see
// https://github.com/zawy12/difficulty-algorithms/issues/3#issuecomment-388386175
//  FTL must be changed to 300 or N*T/20 whichever is higher.
//  FTL in BTC clones is MAX_FUTURE_BLOCK_TIME in chain.h.
//  FTL in Ignition, Numus, and others can be found in main.h as DRIFT.
//  FTL in Zcash & Dash clones need to change the 2*60*60 here:
//  if (block.GetBlockTime() > nAdjustedTime + 2 * 60 * 60)
//  which is around line 3450 in main.cpp in ZEC and validation.cpp in Dash

unsigned int Lwma3CalculateNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const int64_t T = params.nPowTargetSpacing;
    const int64_t N = params.lwmaAveragingWindow;
    const int64_t k = N * (N + 1) * T / 2;
    const int64_t height = pindexLast->nHeight;
    const arith_uint256 powLimit = UintToArith256(params.powLimit);
    
    if (height < N) { return powLimit.GetCompact(); }

    arith_uint256 sumTarget, previousDiff, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t t = 0, j = 0, solvetimeSum = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks. 
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? 
                            block->GetBlockTime() : previousTimestamp + 1;

        int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);
        previousTimestamp = thisTimestamp;

        j++;
        t += solvetime * j; // Weighted solvetime sum.
        arith_uint256 target;
        target.SetCompact(block->nBits);
        sumTarget += target / (k * N);

      //  if (i > height - 3) { solvetimeSum += solvetime; } // LWMA-3,  deprecated
        if (i == height) { previousDiff = target.SetCompact(block->nBits); }
    }
    nextTarget = t * sumTarget;
    
   //  if (solvetimeSum < (8 * T) / 10) { nextTarget = (previousDiff*100)/106; } // LWMA-3,  deprecated
    if (nextTarget > powLimit) { nextTarget = powLimit; }

    return nextTarget.GetCompact();
}

// unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
// {
//     assert(pindexLast != nullptr);
//     unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
//
//     // Only change once per difficulty adjustment interval
//     if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
//     {
//         if (params.fPowAllowMinDifficultyBlocks)
//         {
//             // Special difficulty rule for testnet:
//             // If the new block's timestamp is more than 2* 10 minutes
//             // then allow mining of a min-difficulty block.
//             if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
//                 return nProofOfWorkLimit;
//             else
//             {
//                 // Return the last non-special-min-difficulty-rules-block
//                 const CBlockIndex* pindex = pindexLast;
//                 while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
//                     pindex = pindex->pprev;
//                 return pindex->nBits;
//             }
//         }
//         return pindexLast->nBits;
//     }
//
//     // Go back by what we want to be 14 days worth of blocks
//     int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
//     assert(nHeightFirst >= 0);
//     const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
//     assert(pindexFirst);
//
//     return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
// }
//
// unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
// {
//     if (params.fPowNoRetargeting)
//         return pindexLast->nBits;
//
//     // Limit adjustment step
//     int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
//     if (nActualTimespan < params.nPowTargetTimespan/4)
//         nActualTimespan = params.nPowTargetTimespan/4;
//     if (nActualTimespan > params.nPowTargetTimespan*4)
//         nActualTimespan = params.nPowTargetTimespan*4;
//
//     // Retarget
//     const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
//     arith_uint256 bnNew;
//     bnNew.SetCompact(pindexLast->nBits);
//     bnNew *= nActualTimespan;
//     bnNew /= params.nPowTargetTimespan;
//
//     if (bnNew > bnPowLimit)
//         bnNew = bnPowLimit;
//
//     return bnNew.GetCompact();
// }

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);

    // return Lwma3CalculateNextWorkRequired(pindexLast, params);
    return DarkGravityWave(pindexLast, pblock, params);
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
