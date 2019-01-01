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

#include <ctime> // unix_timestamp()
#include <iostream> // unix_timestamp()

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

long int unix_timestamp()
{
    time_t t = std::time(0);
    long int now = static_cast<long int> (t);
    return now;
}

BOOST_AUTO_TEST_CASE(negativeTimeAttack_test) {
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
    double initDifficulty = (double)GetDifficulty(chain, &blocks[0 + 1]);
    double currentDifficulty = (double)initDifficulty;
    double currentDifficultyRatio = (double)currentDifficulty / (double)initDifficulty;

    printf("\n\n\n\n\n\n\n\n\n\n");
    printf("*** Show mainnetParams\n");
    printf("T = %ld\n", mainnetParams.nPowTargetSpacing);
    printf("N = %ld\n", mainnetParams.difficultyAveragingWindowSize);
    printf("powLimit =\n");
    printf("%s\n", powLimit.ToString().c_str()); // 0x1f07ffff
    printf("%s\n", powLimitFromBits.GetHex().c_str()); // 0x1f07ffff
    printf("\n");
    sleep(1);
    
    // Get Current Timestamp
    long int getCurrentTimestamp = unix_timestamp();
    
    // Get Genesis Interval 
    // int interval = 1541009400;
    long int genesisInterval = getCurrentTimestamp - 1541009400;
    int interval;
    
    // Print
    printf("*** Check Genesis\n");
    printf("%-7s %-13s %-13s %-24s %-7s %-14s\n", "Block", "Target(uint)", "Target(hex)", "Diff(double)", "Diff(%)", "Interval");
    printf("%-7d %-13u %-13x %-24.16g %-7.3f %-14d\n", i, (unsigned)nBits, (unsigned)nBits, (double)currentDifficulty, (double)currentDifficultyRatio, (int)genesisInterval);
    // printf("%-7s %s\n", "", powLimitFromBits.ToString().c_str());
    printf("\n");
    sleep(1);
    
    // BEGIN - First Window
    // Attack Interval 
    interval = 15;
    printf("*** First Window\n");
    printf("%-7s %-13s %-13s %-24s %-7s %-14s\n", "Block", "Target(uint)", "Target(hex)", "Diff(double)", "Diff(%)", "Interval");
    sleep(1);
    for (i = 1; i <= 200+0; i++) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        
        // nBits
        nBits = DarkGravityWave(&blocks[i - 1], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
        
        // Calculate Current Block
        blocks[i] = GetBlockIndex(&blocks[i - 1], interval, nBits); // 0 is Interval
        
        // Make Next Block
        blocks[i + 1] = GetBlockIndex(&blocks[i + 0], interval, nBits); // 0 is Interval
        
        // GetDifficulty from Next Block
        chain = CreateChainWithNbits((unsigned)nBits);
        currentDifficulty = (double)GetDifficulty(chain, &blocks[i + 1]);
        currentDifficultyRatio = (double)currentDifficulty / (double)initDifficulty;
        
        // Print
        printf("%-7d %-13u %-13x %-24.16g %-7.3f %-14d\n", i, (unsigned)nBits, (unsigned)nBits, (double)currentDifficulty, (double)currentDifficultyRatio, (int)interval);
        // printf("%-7s %s\n", "", powLimitFromBits.ToString().c_str());
        
        // Check
        BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
        BOOST_CHECK_EQUAL( currentDifficulty, initDifficulty ); // 1.907323166912278e-06
    }
    printf("\n*** First Window Filled\n");
    sleep(1);
    // END - First Window
    
    // BEGIN - ATTACK
    // Attack Interval 
    // interval = -9999;
    printf("\n*** ATTACK: NegativeRandom\n");
    printf("%-7s %-13s %-13s %-24s %-7s %-14s\n", "Block", "Target(uint)", "Target(hex)", "Diff(double)", "Diff(%)", "Interval");
    sleep(1);
    for (i = i + 1; i <= 200+9999; i++) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
        
        // Changing Attack Interval 
        int varInterval;
        // output = min + (rand() % static_cast<int>(max - min + 1))
        int min = -15;
        int max = 15;
        varInterval = min + (rand() % static_cast<int>(max - min + 1));
        
        // nBits
        nBits = DarkGravityWave(&blocks[i - 1], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
        
        // Calculate Current Block
        blocks[i] = GetBlockIndex(&blocks[i - 1], varInterval, nBits); // 0 is Interval
        
        // Make Next Block
        blocks[i + 1] = GetBlockIndex(&blocks[i + 0], varInterval, nBits); // 0 is Interval
        
        // GetDifficulty from Next Block
        chain = CreateChainWithNbits((unsigned)nBits);
        currentDifficulty = (double)GetDifficulty(chain, &blocks[i + 1]);
        currentDifficultyRatio = (double)currentDifficulty / (double)initDifficulty;
        
        // Print
        printf("%-7d %-13u %-13x %-24.16g %-7.3f %-14d\n", i, (unsigned)nBits, (unsigned)nBits, (double)currentDifficulty, (double)currentDifficultyRatio, (int)varInterval);
        // printf("%-7s %s\n", "", powLimitFromBits.ToString().c_str());
        
        // Break Test If Fully Restored
        // if ( (unsigned)nBits == powLimitBits && (double)currentDifficulty == initDifficulty) {
        if ( (double)currentDifficultyRatio >= 2.0 ) {
            printf("\n*** currentDifficultyRatio >= 2.0\n");
            sleep(1);
            break;
        }
    }
    // Check
    // BOOST_CHECK_EQUAL( nBits, 486619892 ); // 1d013af4
    BOOST_CHECK( currentDifficultyRatio >= 2.0 );
    // BOOST_CHECK_EQUAL( nBits, 504667824 ); // 1e149eb0
    // BOOST_CHECK_EQUAL( currentDifficulty, 0.00018943813991848115 );
    // Print
    printf("\n*** ATTACK: NegativeRandom is finished\n");
    sleep(1);
    // END - ATTACK
    
    // BEGIN - DEFENSE
    // Attack Interval 
    // interval = 0;
    printf("\n*** DEFENSE: Random\n");
    printf("%-7s %-13s %-13s %-24s %-7s %-14s\n", "Block", "Target(uint)", "Target(hex)", "Diff(double)", "Diff(%)", "Interval");
    sleep(1);
    for (i = i + 1; i <= 200+9999+9999; i++) {
        // Check pindexLast is NOT nullptr!
        BOOST_CHECK( &blocks[i - 1] != nullptr );
        assert( &blocks[i - 1] != nullptr);
    
        // Changing Attack Interval 
        int varInterval;
        // output = min + (rand() % static_cast<int>(max - min + 1))
        int min = -15;
        int max = 150;
        varInterval = min + (rand() % static_cast<int>(max - min + 1));
    
        // nBits
        nBits = DarkGravityWave(&blocks[i - 1], NULL, chainParams->GetConsensus());
        powLimitFromBits.SetCompact((unsigned)nBits, &fNegative, &fOverflow);
    
        // Calculate Current Block
        blocks[i] = GetBlockIndex(&blocks[i - 1], varInterval, nBits); // 0 is Interval
    
        // Make Next Block
        blocks[i + 1] = GetBlockIndex(&blocks[i + 0], varInterval, nBits); // 0 is Interval
    
        // GetDifficulty from Next Block
        chain = CreateChainWithNbits((unsigned)nBits);
        currentDifficulty = (double)GetDifficulty(chain, &blocks[i + 1]);
        currentDifficultyRatio = (double)currentDifficulty / (double)initDifficulty;
    
        // Print
        printf("%-7d %-13u %-13x %-24.16g %-7.3f %-14d\n", i, (unsigned)nBits, (unsigned)nBits, (double)currentDifficulty, (double)currentDifficultyRatio, (int)varInterval);
        // printf("%-7s %s\n", "", powLimitFromBits.ToString().c_str());
    
        // Break Test If Fully Restored
        if ( (unsigned)nBits == powLimitBits && (double)currentDifficulty == initDifficulty) {
            printf("\n*** currentDifficulty == initDifficulty\n");
            sleep(1);
            break;
        }
    }
    // Check
    BOOST_CHECK_EQUAL( nBits, powLimitBits ); // 0x1f07ffff
    BOOST_CHECK_EQUAL( currentDifficulty, initDifficulty ); // 1.907323166912278e-06
    // Print
    printf("\n*** DEFENSE: Random is finished\n");
    sleep(1);
    // END - DEFENSE
}

/*
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
*/

/*
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
*/

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
