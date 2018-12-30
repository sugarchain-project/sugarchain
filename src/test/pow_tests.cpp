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

static CBlockIndex GetBlockIndex(CBlockIndex *pindexPrev, int64_t nTimeInterval, uint32_t nBits) {
    CBlockIndex block;
    block.pprev = pindexPrev;
    block.nHeight = pindexPrev->nHeight + 1;
    block.nTime = pindexPrev->nTime + nTimeInterval;
    block.nBits = nBits;

    block.nChainWork = pindexPrev->nChainWork + GetBlockProof(block);
    return block;
}

/* BEGIN - GETDIFFICULTY */
/* Calculate the difficulty for a given block index,
 * or the block index of the given chain.
 */
double GetDifficulty(const CChain& chain, const CBlockIndex* blockindex)
{
    if (blockindex == nullptr)
    {
        if (chain.Tip() == nullptr)
            return 1.0;
        else
            blockindex = chain.Tip();
    }

    int nShift = (blockindex->nBits >> 24) & 0xff;
    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;
}

// CChain CreateChainWithNbits(uint32_t nbits)
// {
//     CBlockIndex* block_index = new CBlockIndex();
//     block_index->nHeight = 0;
//     block_index->nTime = 1541009400;
//     block_index->nBits = 0x1f07fffe;
//     CChain chain;
//     chain.SetTip(block_index);
//     return chain;
// }

CBlockIndex* CreateBlockIndexWithNbits(uint32_t nbits)
{
    CBlockIndex* block_index = new CBlockIndex();
    block_index->nHeight = 0;
    block_index->nTime = 1541009400;
    block_index->nBits = 0x1f07fffe;
    return block_index;
}

CChain CreateChainWithNbits(uint32_t nbits)
{
    CBlockIndex* block_index = CreateBlockIndexWithNbits(nbits);
    CChain chain;
    chain.SetTip(block_index);
    return chain;
}

BOOST_AUTO_TEST_CASE(getdifficulty_test) {
    // Copyright (c) 2018 cryptozeny of the Sugarchain Core developers
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params &mainnetParams = chainParams->GetConsensus();

    // int maxBlockIndex = 2147483647 / 1024;
    int maxBlockIndex = INT_MAX / 1024;
    std::vector<CBlockIndex> blocks(maxBlockIndex);
    
    // Block counter.
    int i = 0;
    
    // powLimit
    const arith_uint256 powLimit = UintToArith256(chainParams->GetConsensus().powLimit);
    uint32_t powLimitBits = powLimit.GetCompact();

    // SetCompact
    // https://en.bitcoin.it/wiki/Difficulty
    // https://en.bitcoin.it/wiki/Target
    arith_uint256 powLimitFromBits;
    uint32_t nBits = powLimitBits;
    uint32_t nBitsFuture;
    bool fNegative;
    bool fOverflow;
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff

    // BEGIN - Check nBits
    // arith_uint256 left = UintToArith256(uint256S("0007ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 left = powLimit;
    uint32_t leftBits = left.GetCompact();
    // arith_uint256 right = UintToArith256(uint256S("0007ffff00000000000000000000000000000000000000000000000000000000"));
    arith_uint256 right = powLimitFromBits;
    uint32_t rightBits = right.GetCompact();
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    BOOST_CHECK_EQUAL( leftBits, rightBits ); // 0x1f07ffff

    // Genesis Block.
    blocks[0] = CBlockIndex();
    blocks[0].nHeight = 0;
    blocks[0].nTime = 1541009400;
    blocks[0].nBits = 0x1f07fffe;
    blocks[0].nChainWork = GetBlockProof(blocks[0]);
    
    // Make First Block
    blocks[1] = GetBlockIndex(&blocks[0], 15, nBits); // 15 is the first interval
    
    // Init Difficulty
    CChain chain = CreateChainWithNbits(nBits); // 0x1f07fffe
    double initDifficulty = GetDifficulty(chain, &blocks[0 + 1]);
    double currentDifficulty = initDifficulty;

    printf("\n\n\n\n\n\n\n\n\n\n");
    printf("*** Show mainnetParams\n");
    printf("T = %ld\n", mainnetParams.nPowTargetSpacing);
    printf("N = %ld\n", mainnetParams.difficultyAveragingWindowSize);
    printf("\n");
    sleep(1);
    
    printf("*** Check Genesis\n");
    printf("%-10s %-16s %-16s %s\n",    "Block", "Target(uint)", "Target(hex)", "Diff(double)");
    printf("%-10d %-16u %-16x %.15e\n", i, (unsigned)powLimitBits, (unsigned)powLimitBits, initDifficulty);
    printf("%-10s %s\n",                "", powLimit.ToString().c_str()); // 0x1f07ffff
    printf("\n");
    sleep(1);
    
    /* BEGIN - First Window */
    printf("*** First Window\n");
    printf("%-10s %-16s %-16s %s\n",    "Block", "Target(uint)", "Target(hex)", "Diff(double)");
    sleep(1);
    for (i = 1; i <= 200+5; i++) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        
        // Init Interval 
        int interval = 0;
        
        // nBits
        nBits = DarkGravityWave(&blocks[i - 1], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
        
        // Calculate Current Block
        blocks[i] = GetBlockIndex(&blocks[i - 1], interval, nBits); // 0 is Interval
        
        // Make Next Block
        blocks[i + 1] = GetBlockIndex(&blocks[i + 0], interval, nBits); // 0 is Interval
        
        // GetDifficulty from Next Block
        chain = CreateChainWithNbits((unsigned)nBits);
        currentDifficulty = GetDifficulty(chain, &blocks[i + 1]);
        
        // Print
        printf("%-10d %-16u %-16x %.15e\n", i, (unsigned)nBits, (unsigned)nBits, currentDifficulty);
        // printf("%-10s %s\n",                "", powLimitFromBits.ToString().c_str());
        
        // Check
        // BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
        // BOOST_CHECK_EQUAL( initDifficulty, currentDifficulty ); // 1.907323166912278e-06
    }
    /* END - First Window */
    
    printf("\n*** First Window Filled\n\n");
    sleep(1);
    
    // // Add one block
    // printf("\n*** Add one block\n");
    // printf("%-10s %-16s %-16s %s\n",    "Block", "Target(uint)", "Target(hex)", "Diff(double)");
    // sleep(1);
    // // i++;
    // // Check pindexLast is NOT nullptr!
    // BOOST_CHECK( &blocks[i - 1] != nullptr );
    // assert( &blocks[i - 1] != nullptr);
    // // Calculate
    // blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
    // nBits = DarkGravityWave(&blocks[i - 1], NULL, chainParams->GetConsensus());
    // powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
    // chain = CreateChainWithNbits((unsigned)nBits);
    // currentDifficulty = GetDifficulty(chain, &blocks[i]);
    // printf("%-10d %-16u %-16x %.15e\n", i, (unsigned)nBits, (unsigned)nBits, currentDifficulty);
    // // printf("%-10s %s\n",                "", powLimitFromBits.ToString().c_str());
    // BOOST_CHECK_EQUAL( nBits, 0x1f02aaaa );
    // // 520268458 == 0002aaaa00000000000000000000000000000000000000000000000000000000
    // 
    // // Add one block
    // printf("\n*** Add one block\n");
    // printf("%-10s %-16s %-16s %s\n",    "Block", "Target(uint)", "Target(hex)", "Diff(double)");
    // sleep(1);
    // i++;
    // // Check pindexLast is NOT nullptr!
    // BOOST_CHECK( &blocks[i - 1] != nullptr );
    // assert( &blocks[i - 1] != nullptr);
    // // Calculate
    // blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
    // nBits = DarkGravityWave(&blocks[i - 1], NULL, chainParams->GetConsensus());
    // powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
    // chain = CreateChainWithNbits((unsigned)nBits);
    // currentDifficulty = GetDifficulty(chain, &blocks[i]);
    // printf("%-10d %-16u %-16x %.15e\n", i, (unsigned)nBits, (unsigned)nBits, currentDifficulty);
    // // printf("%-10s %s\n",                "", powLimitFromBits.ToString().c_str());
    // BOOST_CHECK_EQUAL( nBits, 0x1f02a623 );
    // // 520267299 == 0002a62300000000000000000000000000000000000000000000000000000000
    
    #if 0
    /* BEGIN - SMALL ATACK */
    printf("*** SMALL ATTACK: Add 99999999 blocks: with 0 interval: difficulty higher\n");
    sleep(1);
    i++;
    for (i = i; i <= 99999999; i++ ) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        // Calculate
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        chain = CreateChainWithNbits((unsigned)nBits);
        difficulty = GetDifficulty(chain, &blocks[i]);
        printf("%-12s %-5d %.15e\n",       "difficulty",   i, difficulty);
        // printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        // printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());

        // Break test if the MaxPowLimit is 0
        if ( (unsigned)nBits == 0 ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, 0x1f008d1b );
    // 520129819 == 00008d1b00000000000000000000000000000000000000000000000000000000
    
    printf("\n*** SMALL ATTACK (99999999) is finished\n\n");
    sleep(1);
    /* END - SMALL ATACK */
    #endif
}

#if 0
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
#endif

#if 0
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
    // consensus.difficultyAveragingWindowSize = 200; 
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
#endif

#if 0
BOOST_AUTO_TEST_CASE(cryptozeny_test) {
    // Copyright (c) 2018 cryptozeny of the Sugarchain Core developers
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params &mainnetParams = chainParams->GetConsensus();

    std::vector<CBlockIndex> blocks(99999);
    // Block counter.
    int i = 0;
    
    const arith_uint256 powLimit = UintToArith256(chainParams->GetConsensus().powLimit);
    uint32_t powLimitBits = powLimit.GetCompact();
    // arith_uint256 currentPow = powLimit >> 4;
    // uint32_t initialBits = currentPow.GetCompact();
    
    printf("*** Show mainnetParams\n");
    printf("%-12s %-5ld\n",         "T", mainnetParams.nPowTargetSpacing);
    printf("%-12s %-5ld\n",         "N", mainnetParams.difficultyAveragingWindowSize);
    printf("*** Check Genesis\n");
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

    /* BEGIN - Check nBits */

    /*
    MODELL REFERENCE
    arith_uint256 left = UintToArith256(uint256S("0007ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    uint32_t leftBits = left.GetCompact();
    arith_uint256 right = UintToArith256(uint256S("0007ffff00000000000000000000000000000000000000000000000000000000"));
    uint32_t rightBits = right.GetCompact();
    BOOST_CHECK_EQUAL( leftBits, rightBits ); // 0x1f07ffff
    */

    // arith_uint256 left = UintToArith256(uint256S("0007ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 left = powLimit;
    uint32_t leftBits = left.GetCompact();
    // arith_uint256 right = UintToArith256(uint256S("0007ffff00000000000000000000000000000000000000000000000000000000"));
    arith_uint256 right = powLimitFromBits;
    uint32_t rightBits = right.GetCompact();
    printf("*** Check Genesis nBits\n");
    printf("%-12s %-5d %u / %x\n",  "nBitsA", i, leftBits, leftBits);
    printf("%-12s %-5d %s\n",       "powLimitA",     i, powLimit.ToString().c_str()); // 0x1f07ffff
    printf("%-12s %-5d %u / %x\n",  "nBitsB", i, rightBits, rightBits);
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimitB",    i, powLimitFromBits.GetHex().c_str());
    BOOST_CHECK_EQUAL( leftBits, rightBits ); // 0x1f07ffff
    /* END - Check nBits */

    // Genesis block.
    blocks[0] = CBlockIndex();
    blocks[0].nHeight = 0;
    blocks[0].nTime = 1541009400;
    blocks[0].nBits = powLimitBits;

    blocks[0].nChainWork = GetBlockProof(blocks[0]);

    // Create the first window for lwma, with blocks every 15 seconds.
    // consensus.difficultyAveragingWindowSize = 200; 
    // N=200 for T=15: Lwma3CalculateNextWorkRequired
    
    /* Begin - First Window */
    for (i = 1; i < 201; i++) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 15, powLimitBits); // 0x1f07ffff
    }
    uint32_t nBits = Lwma3CalculateNextWorkRequired(&blocks[201], chainParams->GetConsensus());

    // Last block for the first window: still same
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15+1, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[201] Last block for the first window: still same\n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** filling the first window is finished \n");
    BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
    // BOOST_CHECK_EQUAL( powLimitBits.c_str(), powLimitFromBits.GetHex().c_str() ); // 0x1f07ffff // FIXME.SUGAR
    /* End - First Window */
    
    // Add one block: still same
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15+1, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[202] Add one block: still same\n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** \n");
    BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
    
    // Add one block: a little bit higher
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15-1.0000000000000010, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[203] Add one block: a little bit higher\n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** \n");
    BOOST_CHECK_EQUAL( nBits, 0x1f07fff9 );
    // 0x1f07fff9 == 520617977 == 0007fff900000000000000000000000000000000000000000000000000000000

    // Add one block: a little bit lower: back to powLimit
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15*99, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[204] Add one block: a little bit lower: back to powLimit\n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** \n");
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    
    /* BEGIN - SMALL ATACK */
    // Add 5 blocks: small attack: with 0 interval
    printf("*** SMALL ATTACK: Add 5 blocks: attack: with 0 interval: still same\n");
    for ( int j = 0; j < 5; j++ ) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
        // printf("*** block[%d] \n", i-1);
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
        BOOST_CHECK_EQUAL( nBits, powLimitBits );
    }
    printf("*** \n");

    // Add one block: higher with normal interval
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[210] LAST SMALL ATTACK: Add one block: higher with normal interval\n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** SMALL ATTACK is finished\n");
    BOOST_CHECK_EQUAL( nBits, 0x1f07fe58 );
    // 0x1f07fe58 == 520617560 == 0007fe5800000000000000000000000000000000000000000000000000000000
    /* END - SMALL ATACK */
    
    // Add one block: lower: back to powLimit
    blocks[i] = GetBlockIndex(&blocks[i - 1], 15*999, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    printf("*** block[211] Add one block: lower: back to powLimit\n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** \n");
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    
    /* BEGIN - MEDIUM ATACK */
    // Add 100 blocks: small attack: with 0 interval
    printf("*** MEDIUM ATTACK: Add 100 blocks: attack: with 0 interval: higher\n");
    for ( int j = 0; j < 100; j++ ) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    }
    BOOST_CHECK_EQUAL( nBits, 0x1f021c42 );
    // 0x1f021c42 == 520232002 == 00021c4200000000000000000000000000000000000000000000000000000000
    printf("*** MEDIUM ATTACK is finished\n");
    /* END - SMALL ATACK */
    
    /* BEGIN - HUGE ATACK */
    printf("*** HUGE ATTACK: Add 7412 blocks: attack: with 0 interval: insanely higher\n");
    for ( int j = 0; j < 7412; j++ ) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    }
    BOOST_CHECK_EQUAL( nBits, 0xc0deca3 );
    // 0xc0deca3 == 202239139 == 00000000000000000000000000000000000000000deca3000000000000000000
    printf("*** HUGE ATTACK is finished\n");
    /* END - HUGE ATACK */
}
#endif

#if 0
BOOST_AUTO_TEST_CASE(lwmaRidiculous_test) {
    // Copyright (c) 2018 cryptozeny of the Sugarchain Core developers
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params &mainnetParams = chainParams->GetConsensus();

    // int maxBlockIndex = 999999;
    // int maxBlockIndex = 2147483647 / 1024;
    int maxBlockIndex = INT_MAX / 1024;
    std::vector<CBlockIndex> blocks(maxBlockIndex);
    // Block counter.
    int i = 0;

    const arith_uint256 powLimit = UintToArith256(chainParams->GetConsensus().powLimit);
    uint32_t powLimitBits = powLimit.GetCompact();

    /* BEGIN - SetCompact */
    // https://en.bitcoin.it/wiki/Difficulty
    // https://en.bitcoin.it/wiki/Target
    arith_uint256 powLimitFromBits;
    bool fNegative;
    bool fOverflow;
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    /* END - SetCompact */

    /* BEGIN - Check nBits */
    // arith_uint256 left = UintToArith256(uint256S("0007ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 left = powLimit;
    uint32_t leftBits = left.GetCompact();
    // arith_uint256 right = UintToArith256(uint256S("0007ffff00000000000000000000000000000000000000000000000000000000"));
    arith_uint256 right = powLimitFromBits;
    uint32_t rightBits = right.GetCompact();
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    BOOST_CHECK_EQUAL( leftBits, rightBits ); // 0x1f07ffff
    /* END - Check nBits */

    // Genesis block.
    blocks[0] = CBlockIndex();
    blocks[0].nHeight = 0;
    blocks[0].nTime = 1541009400;
    blocks[0].nBits = powLimitBits;
    blocks[0].nChainWork = GetBlockProof(blocks[0]);

    uint32_t nBits = Lwma3CalculateNextWorkRequired(&blocks[mainnetParams.difficultyAveragingWindowSize + 1], chainParams->GetConsensus());

    /* BEGIN - First Window */
    // mainnetParams.difficultyAveragingWindowSize = 200
    // mainnetParams.nPowTargetSpacing = 15
    for (i = 1; i <= mainnetParams.difficultyAveragingWindowSize + 1; i++) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], mainnetParams.nPowTargetSpacing, powLimitBits); // 0x1f07ffff
    }

    blocks[i] = GetBlockIndex(&blocks[i - 1], mainnetParams.nPowTargetSpacing+1, nBits);
    nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff

    printf("*** BEGIN - Check First Window\n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** END - Check First Window\n");
    BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
    /* END - First Window */

    /* BEGIN - HUGE ATACK */
    printf("*** HUGE ATTACK: Add many blocks: attack: with 0 interval: insanely higher\n");
    for ( int j = 0 + i; j < maxBlockIndex; j++ ) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = Lwma3CalculateNextWorkRequired(&blocks[i++], chainParams->GetConsensus());
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());

        if ( (unsigned)nBits == 0 ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    // 0xc0deca3 == 202239139 == 00000000000000000000000000000000000000000deca3000000000000000000
    printf("*** HUGE ATTACK is finished\n");
    /* END - HUGE ATACK */
    sleep(1);
}
#endif

#if 0
BOOST_AUTO_TEST_CASE(dgwRidiculous_test) {
    // Copyright (c) 2018 cryptozeny of the Sugarchain Core developers
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params &mainnetParams = chainParams->GetConsensus();

    // int maxBlockIndex = 999999;
    // int maxBlockIndex = 2147483647 / 1024;
    int maxBlockIndex = INT_MAX / 1024;
    std::vector<CBlockIndex> blocks(maxBlockIndex);
    // Block counter.
    int i = 0;
    
    const arith_uint256 powLimit = UintToArith256(chainParams->GetConsensus().powLimit);
    uint32_t powLimitBits = powLimit.GetCompact();

    /* BEGIN - SetCompact */
    // https://en.bitcoin.it/wiki/Difficulty
    // https://en.bitcoin.it/wiki/Target
    arith_uint256 powLimitFromBits;
    bool fNegative;
    bool fOverflow;
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    /* END - SetCompact */

    /* BEGIN - Check nBits */
    // arith_uint256 left = UintToArith256(uint256S("0007ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 left = powLimit;
    uint32_t leftBits = left.GetCompact();
    // arith_uint256 right = UintToArith256(uint256S("0007ffff00000000000000000000000000000000000000000000000000000000"));
    arith_uint256 right = powLimitFromBits;
    uint32_t rightBits = right.GetCompact();
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    BOOST_CHECK_EQUAL( leftBits, rightBits ); // 0x1f07ffff
    /* END - Check nBits */

    // Genesis block.
    blocks[0] = CBlockIndex();
    blocks[0].nHeight = 0;
    blocks[0].nTime = 1541009400;
    blocks[0].nBits = powLimitBits;
    blocks[0].nChainWork = GetBlockProof(blocks[0]);

    // unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
    // {
    //     assert(pindexLast != nullptr);
    // 
    //     // return Lwma3CalculateNextWorkRequired(pindexLast, params);
    //     return DarkGravityWave(pindexLast, pblock, params);
    // }
    
    uint32_t nBits = DarkGravityWave(&blocks[24 + 1], NULL, chainParams->GetConsensus());
    
    /* BEGIN - First Window */
    // mainnetParams.difficultyAveragingWindowSize = 200
    // mainnetParams.nPowTargetSpacing = 15
    for (i = 1; i <= 24 + 1; i++) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], mainnetParams.nPowTargetSpacing, powLimitBits); // 0x1f07ffff
    }
    
    blocks[i] = GetBlockIndex(&blocks[i - 1], mainnetParams.nPowTargetSpacing+1, nBits);
    nBits = DarkGravityWave(&blocks[i++], NULL, chainParams->GetConsensus());
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    
    printf("*** BEGIN - Check First Window\n");
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
    printf("*** END - Check First Window\n");
    BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
    /* END - First Window */

    /* BEGIN - HUGE ATACK */
    printf("*** HUGE ATTACK: Add many blocks: attack: with 0 interval: insanely higher\n");
    for ( int j = 0 + i; j < maxBlockIndex; j++ ) {
        blocks[i] = GetBlockIndex(&blocks[i - 1], 1, nBits);
        nBits = DarkGravityWave(&blocks[i++], NULL, chainParams->GetConsensus());
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i-1, (unsigned)nBits, (unsigned)nBits);
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %s\n",       "powLimit2",    i-1, powLimitFromBits.GetHex().c_str());
        
        if ( (unsigned)nBits == 0 ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    // 0xc0deca3 == 202239139 == 00000000000000000000000000000000000000000deca3000000000000000000
    printf("*** HUGE ATTACK is finished\n");
    /* END - HUGE ATACK */
}
#endif

#if 0
BOOST_AUTO_TEST_CASE(cryptozenyDarkGravityWave_test) {
    // Copyright (c) 2018 cryptozeny of the Sugarchain Core developers
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params &mainnetParams = chainParams->GetConsensus();

    // int maxBlockIndex = 2147483647 / 1024;
    int maxBlockIndex = INT_MAX / 1024;
    std::vector<CBlockIndex> blocks(maxBlockIndex);
    
    // Block counter.
    int i = 0;
    
    // powLimit
    const arith_uint256 powLimit = UintToArith256(chainParams->GetConsensus().powLimit);
    uint32_t powLimitBits = powLimit.GetCompact();

    // SetCompact
    // https://en.bitcoin.it/wiki/Difficulty
    // https://en.bitcoin.it/wiki/Target
    arith_uint256 powLimitFromBits;
    bool fNegative;
    bool fOverflow;
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff

    // BEGIN - Check nBits
    // arith_uint256 left = UintToArith256(uint256S("0007ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));
    arith_uint256 left = powLimit;
    uint32_t leftBits = left.GetCompact();
    // arith_uint256 right = UintToArith256(uint256S("0007ffff00000000000000000000000000000000000000000000000000000000"));
    arith_uint256 right = powLimitFromBits;
    uint32_t rightBits = right.GetCompact();
    powLimitFromBits.SetCompact((unsigned)powLimitBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
    BOOST_CHECK_EQUAL( leftBits, rightBits ); // 0x1f07ffff

    // Genesis block.
    blocks[0] = CBlockIndex();
    blocks[0].nHeight = 0;
    blocks[0].nTime = 1541009400;
    blocks[0].nBits = powLimitBits;
    blocks[0].nChainWork = GetBlockProof(blocks[0]);
    
    // Shorting the variable names
    int firstWindowSize = mainnetParams.difficultyAveragingWindowSize;
    int interval = mainnetParams.nPowTargetSpacing;
    
    // Init nBits
    // uint32_t nBits = DarkGravityWave(&blocks[0], NULL, chainParams->GetConsensus());
    uint32_t nBits = powLimitBits;

    printf("\n\n\n\n\n\n\n\n\n\n");
    printf("*** Show mainnetParams\n");
    printf("%-12s %-5ld\n",         "T", mainnetParams.nPowTargetSpacing);
    printf("%-12s %-5ld\n\n",       "N", mainnetParams.difficultyAveragingWindowSize);
    sleep(1);
    
    printf("*** Check Genesis\n");
    printf("%-12s %-5s %s\n",       "Parameter", "Block", "Value");
    printf("%-12s %-5d %u / %x\n",  "powLimitBits", i, (unsigned)powLimitBits, (unsigned)powLimitBits);
    printf("%-12s %-5d %s\n\n",     "powLimit",     i, powLimit.ToString().c_str()); // 0x1f07ffff
    sleep(1);
    
    /* BEGIN - First Window */
    for (i = 1; i <= 199; i++) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        
        // Loop
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());
        BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
    }
    // /* END - First Window */
    
    printf("\n*** First Window Filled\n\n");
    sleep(1);
    
    // Add one block
    // i++;
    // Check pindexLast is NOT nullptr!
    BOOST_CHECK( &blocks[i - 1] != nullptr );
    assert( &blocks[i - 1] != nullptr);
    // Calculate
    blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
    nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
    printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());
    BOOST_CHECK_EQUAL( nBits, 0x1f02aaaa );
    // 520268458 == 0002aaaa00000000000000000000000000000000000000000000000000000000

    printf("\n*** Add one block\n\n");
    sleep(1);
    
    // Add one block
    i++;
    // Check pindexLast is NOT nullptr!
    BOOST_CHECK( &blocks[i - 1] != nullptr );
    assert( &blocks[i - 1] != nullptr);
    // Calculate
    blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
    nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
    powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
    printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
    printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());
    BOOST_CHECK_EQUAL( nBits, 0x1f02a623 );
    // 520267299 == 0002a62300000000000000000000000000000000000000000000000000000000
    
    printf("\n*** Add one block\n\n");
    sleep(1);
    
    /* BEGIN - SMALL ATACK */
    printf("*** SMALL ATTACK: Add 200 blocks: with 0 interval: difficulty higher\n");
    sleep(1);
    i++;
    for (i = i; i <= 401; i++ ) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        // Calculate
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());

        // Break test if the MaxPowLimit is 0
        if ( (unsigned)nBits == 0 ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, 0x1f008d1b );
    // 520129819 == 00008d1b00000000000000000000000000000000000000000000000000000000
    
    printf("\n*** SMALL ATTACK (200) is finished\n\n");
    sleep(1);
    /* END - SMALL ATACK */
    
    /* BEGIN - SMALL RESTORE */
    printf("*** SMALL RESTORE: Add some blocks: with slower (15*10) interval: difficulty is going back to powLimit\n");
    sleep(1);
    // for (i < maxBlockIndex; i++ ) {
    for (i = i; i <= 508; i++ ) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        // Calculate
        blocks[i] = GetBlockIndex(&blocks[i - 1], 15*10, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());

        // Break test if the MaxPowLimit is 0
        if ( (unsigned)nBits == powLimitBits ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    
    printf("\n*** SMALL RESTORE is finished\n\n");
    sleep(1);
    /* END - SMALL RESTORE */
    
    /* BEGIN - HUGE ATACK */
    printf("*** HUGE ATTACK: Add 4000 blocks: with 0 interval: difficulty insanely higher\n");
    sleep(1);
    i++;
    for (i = i; i <= 4508; i++ ) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        // Calculate
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());

        // Break test if the MaxPowLimit is 0
        if ( (unsigned)nBits == 0 ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, 0x1836a38c );
    // 406233996 == 000000000000000036a38c000000000000000000000000000000000000000000
    
    printf("\n*** HUGE ATTACK (4000) is finished\n\n");
    sleep(1);
    /* END - HUGE ATACK */
    
    /* BEGIN - HUGE RESTORE */
    printf("*** HUGE RESTORE: Add some blocks: with slower (15*100) interval: difficulty is going back to powLimit\n");
    sleep(1);
    // for (i < maxBlockIndex; i++ ) {
    for (i = i; i <= 6996; i++ ) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        // Calculate
        blocks[i] = GetBlockIndex(&blocks[i - 1], 15*100, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());

        // Break test if the nBits is powLimit
        if ( (unsigned)nBits == powLimitBits ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    
    printf("\n*** HUGE RESTORE is finished\n\n");
    sleep(1);
    /* END - HUGE RESTORE */
    
    /* BEGIN - SMALL DELAY */
    printf("*** SMALL DELAY: Add 300 blocks: with long delay (15*9999) interval: difficulty must stay at powLimit\n");
    sleep(1);
    for (i = i; i <= 7296; i++ ) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        // Calculate
        blocks[i] = GetBlockIndex(&blocks[i - 1], 15*9999, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());

        // Break test if the MaxPowLimit is 0
        if ( (unsigned)nBits == 0 ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    
    printf("\n*** SMALL DELAY (300) is finished\n\n");
    sleep(1);
    /* END - SMALL DELAY */
    
    /* BEGIN - HUGE DELAY */
    printf("*** HUGE DELAY: Add 6000 blocks: with long delay (15*9999999999999999) interval: difficulty must stay at powLimit\n");
    sleep(1);
    for (i = i; i <= 13296; i++ ) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        // Calculate
        blocks[i] = GetBlockIndex(&blocks[i - 1], 15*9999999999999999, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());

        // Break test if the MaxPowLimit is 0
        if ( (unsigned)nBits == 0 ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    
    printf("\n*** HUGE DELAY (6000) is finished\n\n");
    sleep(1);
    /* END - SMALL DELAY */
    
    /* BEGIN - INCREDIBLY HUGE ATACK */
    printf("*** INCREDIBLY HUGE ATTACK: Add 17481-200 blocks: with 0 interval: difficulty much insanely higher\n");
    sleep(1);
    i++;
    // for (i = i; i <= 28296; i++ ) {
    for (i = i; i <= 30777-200; i++ ) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        // Calculate
        blocks[i] = GetBlockIndex(&blocks[i - 1], 0, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());

        // Break test if the MaxPowLimit is 0
        if ( (unsigned)nBits == 0 ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, 0x200c900 );
    // 33605888 == 00000000000000000000000000000000000000000000000000000000000000c9
    
    printf("\n*** INCREDIBLY HUGE ATTACK (17481-200) is finished\n\n");
    sleep(1);
    /* END - INCREDIBLY HUGE ATACK */
    
    /* BEGIN - INCREDIBLY HUGE RESTORE */
    printf("*** INCREDIBLY HUGE RESTORE: Add some blocks: with slower (15*9999999999999999) interval: difficulty is going back to powLimit\n");
    sleep(1);
    // for (i < maxBlockIndex; i++ ) {
    for (i = i; i <= 30777-200+99999999; i++ ) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        // Calculate
        blocks[i] = GetBlockIndex(&blocks[i - 1], 15*9999999999999999, nBits);
        nBits = DarkGravityWave(&blocks[i], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow); // powLimitBits == 0x1f07ffff
        printf("%-12s %-5d %u / %x\n",  "currentBits",  i, (unsigned)nBits, (unsigned)nBits);
        printf("%-12s %-5d %s\n",       "powLimit2",    i, powLimitFromBits.GetHex().c_str());
    
        // Break test if the nBits is powLimit
        if ( (unsigned)nBits == powLimitBits ) {
            break;
        }
    }
    BOOST_CHECK_EQUAL( nBits, powLimitBits );
    
    printf("\n*** INCREDIBLY HUGE RESTORE is finished\n\n");
    sleep(1);
    /* END - INCREDIBLY HUGE RESTORE */
}
#endif

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
