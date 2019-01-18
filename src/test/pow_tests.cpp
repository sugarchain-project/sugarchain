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

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    /*
    2019-01-18 21:57:00 UpdateTip: new best=5697d5baaf811935549afaebbd8daa1fb955c5f3dae2bb34c8a311321981f486 height=21428 version=0x20000000 log2_work=24.356938 tx=21429 date='2019-01-18 21:56:55' progress=1.000000 cache=0.0MiB(9txo)
2019-01-18 21:57:01

    bnAvg=0030245099191919191919191919191919191919191919191919191919191919 nLastBlockTime=1547848604 nFirstBlockTime=1547846198 params=0x561663516c70

    2019-01-18 21:57:01   nActualTimespan = 2406  before dampening
    2019-01-18 21:57:01   nActualTimespan = 2514  before bounds
    2019-01-18 21:57:01 GetNextWorkRequired RETARGET
    2019-01-18 21:57:01 params.AveragingWindowTimespan() = 2550    nActualTimespan = 2514
    2019-01-18 21:57:01 Timespan ratio: 2550 / 2514 = 1.01
    2019-01-18 21:57:01 Current average: 1f302450  0030245099191919191919191919191919191919191919191919191919191919
    2019-01-18 21:57:01 After:  1f2f7653  002f7653130577ea5ccf41b426990b7df062d547ba2c9f1183f668db4dc029ea
    2019-01-18 21:57:01 

    bnNew.GetCompact()=523204179
    */

    const auto chainParams = CreateChainParams(CBaseChainParams::TESTNET); // height=21428
    arith_uint256 bnAvg = arith_uint256("0030245099191919191919191919191919191919191919191919191919191919");
    int64_t nLastBlockTime = 1547848604;
    int64_t nFirstBlockTime = 1547846198;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(bnAvg, nLastBlockTime, nFirstBlockTime, chainParams->GetConsensus()), 523204179);
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    /*
    bnAvg=0080800000000000000000000000000000000000000000000000000000000000 nLastBlockTime=1547748480 nFirstBlockTime=1541009401 params=0x55767541a610

    2019-01-18 22:18:47   nActualTimespan = 6739079  before dampening
    2019-01-18 22:18:47   nActualTimespan = 1686682  before bounds
    2019-01-18 22:18:47 GetNextWorkRequired RETARGET
    2019-01-18 22:18:47 params.AveragingWindowTimespan() = 2550    nActualTimespan = 3366
    2019-01-18 22:18:47 Timespan ratio: 2550 / 3366 = [31;1m0.758[0m
    2019-01-18 22:18:47 Current average: 20008080  0080800000000000000000000000000000000000000000000000000000000000
    2019-01-18 22:18:47 After:  20008080  0080808080808080808080808080808080808080808080808080808080808080
    2019-01-18 22:18:47 

    bnNew.GetCompact()=536903808
    */

    const auto chainParams = CreateChainParams(CBaseChainParams::TESTNET); // height=511 (N=510)
    arith_uint256 bnAvg = arith_uint256("0080800000000000000000000000000000000000000000000000000000000000");
    int64_t nLastBlockTime = 1547748480;
    int64_t nFirstBlockTime = 1541009401;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(bnAvg, nLastBlockTime, nFirstBlockTime, chainParams->GetConsensus()), 536903808);
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    /*
    bnAvg=003fd04d68686868686868686868686868686868686868686868686868686868 nLastBlockTime=1547795937 nFirstBlockTime=1547793171 params=0x55767541a610

    2019-01-18 22:20:01   nActualTimespan = 2766  before dampening
    2019-01-18 22:20:01   nActualTimespan = 2604  before bounds
    2019-01-18 22:20:01 GetNextWorkRequired RETARGET
    2019-01-18 22:20:01 params.AveragingWindowTimespan() = 2550    nActualTimespan = 2604
    2019-01-18 22:20:01 Timespan ratio: 2550 / 2604 = [31;1m0.979[0m
    2019-01-18 22:20:01 Current average: 1f3fd04d  003fd04d68686868686868686868686868686868686868686868686868686868
    2019-01-18 22:20:01 After:  1f412a3f  00412a3f62c9ca649865cccd679b68cfd06a9e6bd2d36da16ed5d670a471d604
    2019-01-18 22:20:01 

    bnNew.GetCompact()=524364351
    */

    const auto chainParams = CreateChainParams(CBaseChainParams::TESTNET); // height=???
    arith_uint256 bnAvg = arith_uint256("003fd04d68686868686868686868686868686868686868686868686868686868");
    int64_t nLastBlockTime = 1547795937;
    int64_t nFirstBlockTime = 1547793171;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(bnAvg, nLastBlockTime, nFirstBlockTime, chainParams->GetConsensus()), 524364351);
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    /*
    bnAvg=0080800000000000000000000000000000000000000000000000000000000000 nLastBlockTime=1547748481 nFirstBlockTime=1547747148 params=0x55767541a610

    2019-01-18 22:18:47   nActualTimespan = 1333  before dampening
    2019-01-18 22:18:47   nActualTimespan = 2246  before bounds
    2019-01-18 22:18:47 GetNextWorkRequired RETARGET
    2019-01-18 22:18:47 params.AveragingWindowTimespan() = 2550    nActualTimespan = 2246
    2019-01-18 22:18:47 Timespan ratio: 2550 / 2246 = [31;1m1.14[0m
    2019-01-18 22:18:47 Current average: 20008080  0080800000000000000000000000000000000000000000000000000000000000
    2019-01-18 22:18:47 After:  1f712e47  00712e47e17b14ae47e17b14ae47e17b14ae47e17b14ae47e17b14ae47e17872
    2019-01-18 22:18:47 

    bnNew.GetCompact()=527511111
    */

    const auto chainParams = CreateChainParams(CBaseChainParams::TESTNET); // height=512
    arith_uint256 bnAvg = arith_uint256("0080800000000000000000000000000000000000000000000000000000000000");
    int64_t nLastBlockTime = 1547748481;
    int64_t nFirstBlockTime = 1547747148;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(bnAvg, nLastBlockTime, nFirstBlockTime, chainParams->GetConsensus()), 527511111);
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    // genesis = CreateGenesisBlock(1541009401, 441, 0x20008080, 1, 50 * COIN);
    const auto chainParams = CreateChainParams(CBaseChainParams::TESTNET);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1541009401 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x20008080; /* target ???... */
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
