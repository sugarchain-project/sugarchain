// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <random.h>
#include <util.h>
#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(h4x3rotab_test)
{
    // Copyright (c) 2017-2018 h4x3rotab of the Bitcoin Gold
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(805); // (N * 4) + 5 = (200 * 4) + 5 = 805
    for (int i = 0; i < 805; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1541009400 + i * chainParams->GetConsensus().nPowTargetSpacing; // block:0
        blocks[i].nBits = 0x1f07ffff; // block:0 // 0x1f07ffff = 520617983
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    int bits = Lwma3CalculateNextWorkRequired(&blocks.back(), chainParams->GetConsensus());
    BOOST_CHECK_EQUAL(bits, 0x1f07fffe); // 0x1f07fffe = 520617983(0x1f07ffff) - 1 = 520617982
}

static CBlockIndex GetBlockIndex(CBlockIndex *pindexPrev, int64_t nTimeInterval, uint32_t nBits) {
    CBlockIndex block;
    block.pprev = pindexPrev;
    block.nHeight = pindexPrev->nHeight + 1;
    block.nTime = pindexPrev->nTime + nTimeInterval;
    block.nBits = nBits;

    block.nChainWork = pindexPrev->nChainWork + GetBlockProof(block);
    return block;
}

BOOST_AUTO_TEST_CASE(ishikawa_test) {
    // Copyright (c) 2018 ishikawa-pss9 of the Susucoin Core developers
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);

    std::vector<CBlockIndex> blocks(3000);

    const arith_uint256 powLimit = UintToArith256(chainParams->GetConsensus().powLimit);
    // uint32_t powLimitBits = powLimit.GetCompact();
    arith_uint256 currentPow = powLimit >> 4;
    uint32_t initialBits = currentPow.GetCompact();

    // Genesis block.
    blocks[0] = CBlockIndex();
    blocks[0].nHeight = 0;
    blocks[0].nTime = 1541009400;
    blocks[0].nBits = initialBits;

    blocks[0].nChainWork = GetBlockProof(blocks[0]);

    // Block counter.
    size_t i;

    // Create the first window for lwma, with blocks every 10 minutes.
    // consensus.lwmaAveragingWindow = 200; 
    // N=200 for T=15: Lwma3CalculateNextWorkRequired
    for (i = 1; i < 202; i++) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 600, initialBits); // 0x1f07ffff
    }

    uint32_t nBits =
        Lwma3CalculateNextWorkRequired(&blocks[201], chainParams->GetConsensus());

    // For the first window, with 10 minutes between blocks, the difficulty should be low.
    BOOST_CHECK_EQUAL( nBits, 0x1f02ffff ); // 520421370

    // Add one block far in the future.
    blocks[i] = GetBlockIndex(&blocks[i - 1], 6000, nBits);
    // The difficulty is now a somewhat lower.
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    BOOST_CHECK_EQUAL( nBits, 0x1f031333 ); // 520295219

    // Add another block with a normal timestamp.
    blocks[i] = GetBlockIndex(&blocks[i - 1], 2 * 600 - 6000, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    // The difficulty is now just a little bit lower, again.
    BOOST_CHECK_EQUAL( nBits, 0x1f031f09 ); // 520298249

    // And another block with a regular timestamp.
    blocks[i] = GetBlockIndex(&blocks[i - 1], 600, nBits);
    // The difficulty has lowered yet again, by a fraction.
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    BOOST_CHECK_EQUAL( nBits, 0x1f032ade ); // 520301278

    // Simulate a hash attack, add a window with very low increase.
    for ( int j = 0; j < 10; j++ ) {
        // first, add one block with 0.125 second interval
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0.125, nBits);
        nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
	// then add 20 more with zero second interval
        for ( int k = 0; k < 20; k++ ) {
            blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
            nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
        }
        // and do that ten times.  That gives us 200 block window with very high frequency
        // of blocks.
    }
    // The difficulty is now significantly higher.
    BOOST_CHECK_EQUAL( nBits, 0x1e2eaf51 ); // 506376017

    // Add one more block with a significant delay.
    blocks[i] = GetBlockIndex(&blocks[i - 1], 4 * 3600, nBits);
    // The difficulty has lowered significantly.
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    BOOST_CHECK_EQUAL( nBits, 0x1e577959 ); // 509049177

    // One more block with little less delay.
    blocks[i] = GetBlockIndex(&blocks[i - 1], 2 * 3600, nBits);
    // The difficulty has lowered again.
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    BOOST_CHECK_EQUAL( nBits, 0x1e7f90f4 ); // 511676660
}

BOOST_AUTO_TEST_CASE(sugarchain_test) {
    // Copyright (c) 2018 The Sugarchain Core developers
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params &mainnetParams = chainParams->GetConsensus();

    std::vector<CBlockIndex> blocks(500);
    // Block counter.
    int i = 0;
    
    const arith_uint256 powLimit = UintToArith256(chainParams->GetConsensus().powLimit);
    uint32_t powLimitBits = powLimit.GetCompact();
    // arith_uint256 currentPow = powLimit >> 4;
    // uint32_t initialBits = currentPow.GetCompact();
    
    printf("*** mainnetParams \n");
    printf("%-12s %-5ld \n",        "T", mainnetParams.nPowTargetSpacing);
    printf("%-12s %-5ld \n",        "N", mainnetParams.lwmaAveragingWindow);
    printf("*** block[0] \n");
    printf("%-12s %-5s %s\n",       "Parameter", "Block", "Value");
    printf("%-12s %-5d %u / %x\n",  "powLimitBits", i, (unsigned)powLimitBits, (unsigned)powLimitBits);
    printf("%-12s %-5d %s\n",       "powLimit",     i, powLimit.ToString().c_str()); // 0x1f07ffff

    /* BEGIN - SetCompact */
    // https://en.bitcoin.it/wiki/Difficulty
    // https://en.bitcoin.it/wiki/Target
    arith_uint256 powLimitFromBits;
    bool fNegative;
    bool fOverflow;
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());
    /* END - SetCompact */

    // printf("%-12s %-5d %u\n",       "powLimitA",    i, (unsigned)powLimitBits.SetCompact();
    // printf("%-12s %-5d %s\n",       "powLimit2",    i, ArithToUint256(powLimitBits).ToString().c_str());
    // printf("%-12s %-5d %s\n",       "powLimit3",    i, currentPow.ToString().c_str());
    // printf("%-12s %-5d %s\n",        "currentPow",   i, currentPow.ToString().c_str());
    // printf("%-12s %-5d %u / %x\n",   "currentBits",  i, (unsigned)powLimit.GetCompact(), (unsigned)powLimit.GetCompact());
    // printf("******\n");

    // Genesis block.
    blocks[0] = CBlockIndex();
    blocks[0].nHeight = 0;
    blocks[0].nTime = 1541009400;
    blocks[0].nBits = powLimitBits;

    blocks[0].nChainWork = GetBlockProof(blocks[0]);

    // Create the first window for lwma, with blocks every 10 minutes.
    // consensus.lwmaAveragingWindow = 200; 
    // N=200 for T=15: Lwma3CalculateNextWorkRequired
    
    /* Begin - First Window */
    for (i = 1; i < 201; i++) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 15, powLimitBits); // 0x1f07ffff
    }
    uint32_t nBits = Lwma3CalculateNextWorkRequired(&blocks[201], chainParams->GetConsensus());

    // Last block for the first window: still same
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15+1, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[201] \n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** \n");
    BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
    // BOOST_CHECK_EQUAL( powLimitBits.c_str(), powLimitFromBits.GetHex().c_str() ); // 0x1f07ffff // FIXME.SUGAR
    /* End - First Window */
    
    printf("*** filling first window finished \n");
    printf("*** \n");
    
    // Add one block: still same
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15+1, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[202] \n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** \n");
    BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
    
    // Add one block: a little bit higher
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15-1.0000000000000010, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[203] \n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact(0x1f07fff9, &fNegative, &fOverflow);
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** \n");
    BOOST_CHECK_EQUAL( nBits, 0x1f07fff9 );

    // Add one block: a little bit lower: back to powLimit
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15*100, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[204] \n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** \n");
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    
    // Add 5 blocks: small attack: with 0 interval
    printf("*** SMALL ATTACK \n");
    for ( int j = 0; j < 5; j++ ) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
        // printf("*** block[%d] \n", i-1);
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
        powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
        BOOST_CHECK_EQUAL( nBits, powLimitBits );
    }
    printf("*** \n");

    // Add one block: higher with normal interval
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[210] \n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact(0x1f07fe58, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** \n");
    BOOST_CHECK_EQUAL( nBits, 0x1f07fe58 );
    
    /*
    // Add another block with a normal timestamp.
    blocks[i] = GetBlockIndex(&blocks[i - 1], 2 * 600 - 6000, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    // The difficulty is now just a little bit lower, again.
    BOOST_CHECK_EQUAL( nBits, 0x1f031f09 ); // 520298249

    // And another block with a regular timestamp.
    blocks[i] = GetBlockIndex(&blocks[i - 1], 600, nBits);
    // The difficulty has lowered yet again, by a fraction.
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    BOOST_CHECK_EQUAL( nBits, 0x1f032ade ); // 520301278

    // Simulate a hash attack, add a window with very low increase.
    for ( int j = 0; j < 10; j++ ) {
        // first, add one block with 0.125 second interval
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0.125, nBits);
        nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
	// then add 20 more with zero second interval
        for ( int k = 0; k < 20; k++ ) {
            blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
            nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
        }
        // and do that ten times.  That gives us 200 block window with very high frequency
        // of blocks.
    }
    // The difficulty is now significantly higher.
    BOOST_CHECK_EQUAL( nBits, 0x1e2eaf51 ); // 506376017

    // Add one more block with a significant delay.
    blocks[i] = GetBlockIndex(&blocks[i - 1], 4 * 3600, nBits);
    // The difficulty has lowered significantly.
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    BOOST_CHECK_EQUAL( nBits, 0x1e577959 ); // 509049177

    // One more block with little less delay.
    blocks[i] = GetBlockIndex(&blocks[i - 1], 2 * 3600, nBits);
    // The difficulty has lowered again.
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    BOOST_CHECK_EQUAL( nBits, 0x1e7f90f4 ); // 511676660
    */
}

// /* Test calculation of next difficulty target with no constraints applying */
// BOOST_AUTO_TEST_CASE(get_next_work)
// {
//     const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
//     int64_t nLastRetargetTime = 1261130161; // Block #30240
//     CBlockIndex pindexLast;
//     pindexLast.nHeight = 32255;
//     pindexLast.nTime = 1262152739;  // Block #32255
//     pindexLast.nBits = 0x1d00ffff;
//     BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00d86a);
// }
// 
// /* Test the constraint on the upper bound for next work */
// BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
// {
//     const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
//     int64_t nLastRetargetTime = 1231006505; // Block #0
//     CBlockIndex pindexLast;
//     pindexLast.nHeight = 2015;
//     pindexLast.nTime = 1233061996;  // Block #2015
//     pindexLast.nBits = 0x1d00ffff;
//     BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00ffff);
// }
// 
// /* Test the constraint on the lower bound for actual time taken */
// BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
// {
//     const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
//     int64_t nLastRetargetTime = 1279008237; // Block #66528
//     CBlockIndex pindexLast;
//     pindexLast.nHeight = 68543;
//     pindexLast.nTime = 1279297671;  // Block #68543
//     pindexLast.nBits = 0x1c05a3f4;
//     BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1c0168fd);
// }
// 
// /* Test the constraint on the upper bound for actual time taken */
// BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
// {
//     const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
//     int64_t nLastRetargetTime = 1263163443; // NOTE: Not an actual block time
//     CBlockIndex pindexLast;
//     pindexLast.nHeight = 46367;
//     pindexLast.nTime = 1269211443;  // Block #46367
//     pindexLast.nBits = 0x1c387f6f;
//     BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00e1fd);
// }

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

BOOST_AUTO_TEST_SUITE_END()
