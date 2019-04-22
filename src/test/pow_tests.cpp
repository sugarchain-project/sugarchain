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
    2019-04-22 12:18:06 UpdateTip: new best=be25b4a7c3d1c4cb144f578b0977922b0866e02d8a1327a432dc9fd90878d27c height=6161 version=0x20000000 log2_work=23.325692 tx=6162 date='2019-04-22 12:18:05' progress=1.000000 cache=0.8MiB(5675txo)
    2019-04-22 12:18:06 CreateNewBlock(): block weight: 808 txs: 0 fees: 0 sigops 400
    2019-04-22 12:18:06 

    bnAvg=001c205249494949494949494949494949494949494949494949494949494949 nLastBlockTime=1555935464 nFirstBlockTime=1555932646 params=0x5637617381c0

    2019-04-22 12:18:06   nActualTimespan = 2818  before dampening
    2019-04-22 12:18:06   nActualTimespan = 2617  before bounds
    2019-04-22 12:18:06 GetNextWorkRequired RETARGET
    2019-04-22 12:18:06 params.AveragingWindowTimespan() = 2550    nActualTimespan = 2617
    2019-04-22 12:18:06 Timespan ratio: 2550 / 2617 = [31;1m0.974[0m
    2019-04-22 12:18:06 Current average: 1f1c2052  001c205249494949494949494949494949494949494949494949494949494949
    2019-04-22 12:18:06 After:  1f1cdd81  001cdd819ff15c60ff37087378164e1f8a8f2d6536a1a6447c4db8bd5b936334  521985409
    2019-04-22 12:18:06
    */

    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN); // height=6161
    arith_uint256 bnAvg = arith_uint256("001c205249494949494949494949494949494949494949494949494949494949");
    int64_t nLastBlockTime = 1555935464;
    int64_t nFirstBlockTime = 1555932646;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(bnAvg, nLastBlockTime, nFirstBlockTime, chainParams->GetConsensus()), 521985409); // 0x1f1cdd81
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    /*
    2019-04-22 05:27:41 UpdateTip: new best=fb986f63e1211dcb39a1160effa62319705e5c72bd362724691646389146c247 height=511 version=0x20000000 log2_work=19 tx=512 date='2019-04-22 05:27:40' progress=1.000000 cache=0.0MiB(14txo)
    2019-04-22 05:27:41 AddToWallet 8e0eb41c43c4a4931e28a8dc786342e3d9703be1896047565578b4090871f28d  new
    2019-04-22 05:27:41 CreateNewBlock(): block weight: 808 txs: 0 fees: 0 sigops 400
    2019-04-22 05:27:41 

    bnAvg=003fffff00000000000000000000000000000000000000000000000000000000 nLastBlockTime=1555910839 nFirstBlockTime=1555908929 params=0x5637617381c0

    2019-04-22 05:27:41   nActualTimespan = 1910  before dampening
    2019-04-22 05:27:41   nActualTimespan = 2390  before bounds
    2019-04-22 05:27:41 GetNextWorkRequired RETARGET
    2019-04-22 05:27:41 params.AveragingWindowTimespan() = 2550    nActualTimespan = 2390
    2019-04-22 05:27:41 Timespan ratio: 2550 / 2390 = [31;1m1.07[0m
    2019-04-22 05:27:41 Current average: 1f3fffff  003fffff00000000000000000000000000000000000000000000000000000000
    2019-04-22 05:27:41 After:  1f3bfbfb  003bfbfb0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0548  524024827
    2019-04-22 05:27:41
    */

    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN); // height=511 (N=510)
    arith_uint256 bnAvg = arith_uint256("003fffff00000000000000000000000000000000000000000000000000000000");
    int64_t nLastBlockTime = 1555910839;
    int64_t nFirstBlockTime = 1555908929;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(bnAvg, nLastBlockTime, nFirstBlockTime, chainParams->GetConsensus()), 524024827); // 0x1f3bfbfb
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    /*
    2019-04-22 06:17:08 UpdateTip: new best=66e32f2f15b55ccc519431e94b9baa8546ef464fc26b7c6c1d8ba46fe7812ce5 height=1234 version=0x20000000 log2_work=20.373246 tx=1235 date='2019-04-22 06:17:08' progress=1.000000 cache=0.1MiB(737txo)
    2019-04-22 06:17:08 AddToWallet aab7131d9100234693f3f4f7200343675283d174c335ef601a45d581fb6e0905  new
    2019-04-22 06:17:08 CreateNewBlock(): block weight: 808 txs: 0 fees: 0 sigops 400
    2019-04-22 06:17:08 

    bnAvg=0037e5f3e2626262626262626262626262626262626262626262626262626262 nLastBlockTime=1555913812 nFirstBlockTime=1555911686 params=0x5637617381c0

    2019-04-22 06:17:08   nActualTimespan = 2126  before dampening
    2019-04-22 06:17:08   nActualTimespan = 2444  before bounds
    2019-04-22 06:17:08 GetNextWorkRequired RETARGET
    2019-04-22 06:17:08 params.AveragingWindowTimespan() = 2550    nActualTimespan = 2444
    2019-04-22 06:17:08 Timespan ratio: 2550 / 2444 = [31;1m1.04[0m
    2019-04-22 06:17:08 Current average: 1f37e5f3  0037e5f3e2626262626262626262626262626262626262626262626262626262
    2019-04-22 06:17:08 After:  1f35931b  0035931b8a7797eb722c19398d13cdbadb2eb56f5c7cd05710fe1e71f8b297cc  523604763
    2019-04-22 06:17:08 
    */

    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN); // height=1234
    arith_uint256 bnAvg = arith_uint256("0037e5f3e2626262626262626262626262626262626262626262626262626262");
    int64_t nLastBlockTime = 1555913812;
    int64_t nFirstBlockTime = 1555911686;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(bnAvg, nLastBlockTime, nFirstBlockTime, chainParams->GetConsensus()), 523604763); // 0x1f35931b
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    /*
    2019-04-22 05:27:43 UpdateTip: new best=80c7e76ad10627c7f393b601db2dcd8dd2e470666d6d1477d584741b7cf99c7e height=512 version=0x20000000 log2_work=19.003002 tx=513 date='2019-04-22 05:27:41' progress=1.000000 cache=0.0MiB(15txo)
    2019-04-22 05:27:43 CreateNewBlock(): block weight: 808 txs: 0 fees: 0 sigops 400
    2019-04-22 05:27:43 

    bnAvg=003ffdfaf9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9 nLastBlockTime=1555910839 nFirstBlockTime=1555908929 params=0x5637617381c0

    2019-04-22 05:27:43   nActualTimespan = 1910  before dampening
    2019-04-22 05:27:43   nActualTimespan = 2390  before bounds
    2019-04-22 05:27:43 GetNextWorkRequired RETARGET
    2019-04-22 05:27:43 params.AveragingWindowTimespan() = 2550    nActualTimespan = 2390
    2019-04-22 05:27:43 Timespan ratio: 2550 / 2390 = [31;1m1.07[0m
    2019-04-22 05:27:43 Current average: 1f3ffdfa  003ffdfaf9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9
    2019-04-22 05:27:43 After:  1f3bfa17  003bfa1766c72787e848a90969ca2a8aeb4bac0c6ccd2d8dee4eaf0f6fd02892  524024343
    2019-04-22 05:27:43 
    */

    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN); // height=512
    arith_uint256 bnAvg = arith_uint256("003ffdfaf9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9f9");
    int64_t nLastBlockTime = 1555910839;
    int64_t nFirstBlockTime = 1555908929;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(bnAvg, nLastBlockTime, nFirstBlockTime, chainParams->GetConsensus()), 524024343); // 0x1f3bfa17
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    // genesis = CreateGenesisBlock(1554336000, 2645, 0x1f3fffff, 1, 42.94967296 * COIN);
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1554336000 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x1f3fffff; /* target ???... */
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
