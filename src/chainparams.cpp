// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>

#include <tinyformat.h>
#include <util.h>
#include <utilstrencodings.h>

#include <assert.h>
#include <memory>

#include <chainparamsseeds.h>

#include <arith_uint256.h> // DigiShieldZEC

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);

    // BEGIN - DEBUG - SUGAR

    /*
    // MAINNET
    if (genesis.nTime == 1554336000) {
      printf("\n*** BEGIN - DEBUG: MAINNET\n");
      printf("nTime = %u\n", nTime);
      printf("nNonce = %u\n", nNonce);
      printf("nBits = 0x%x\n", nBits);
      printf("nVersion = %d\n", nVersion);
      printf("genesisReward = %ld\n", genesisReward);
      printf("COIN = %ld\n", COIN);
      printf("*** END - DEBUG\n");
    }
    */

    /*
    // TESTNET
    if (genesis.nTime == 1554336001) {
      printf("\n*** BEGIN - DEBUG: TESTNET\n");
      printf("nTime = %u\n", nTime);
      printf("nNonce = %u\n", nNonce);
      printf("nBits = 0x%x\n", nBits);
      printf("nVersion = %d\n", nVersion);
      printf("genesisReward = %ld\n", genesisReward);
      printf("COIN = %ld\n", COIN);
      printf("*** END - DEBUG\n");
    }
    */

    /*
    // REGTEST
    if (genesis.nTime == 1554336002) {
      printf("\n*** BEGIN - DEBUG: REGTEST\n");
      printf("nTime = %u\n", nTime);
      printf("nNonce = %u\n", nNonce);
      printf("nBits = 0x%x\n", nBits);
      printf("nVersion = %d\n", nVersion);
      printf("genesisReward = %ld\n", genesisReward);
      printf("COIN = %ld\n", COIN);
      printf("*** END - DEBUG\n");
    }
    */

    // END - DEBUG

    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Satoshi Nakamoto 31/Oct/2008 Proof-of-work is essentially one-CPU-one-vote";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

// DigiShieldZEC
const arith_uint256 maxUint = UintToArith256(uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"));

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";

        // SUGAR-HALVING
        // BTC: (was 21000000)
        // around 2 years = pow(5,8)*32 = 390625*32 = 12500000
        // pow(5,8) * pow(2,32) / COIN = 390625 * 4294967296 / 100000000 = INTEGER (16777216.0)
        // pow(5,8) = 390625 is an integer cycle of pow(2,32) = 4294967296
        // 1st halving = 536870912*COIN = 12500000 * 42.94967296
        // Total Supply in COINs (in theory):	1073741824
        // Total Supply in COINs (in actual):	1073741823.87500000
        // Difference: 0.125
        consensus.nSubsidyHalvingInterval = 12500000;

        consensus.BIP16Height = 0;  // always on
        consensus.BIP34Height = 17;
        consensus.BIP34Hash = uint256S("");   // FIXME.SUGAR
        consensus.BIP65Height = 0;  // always on
        consensus.BIP66Height = 0;  // always on

        // GET powLimit by python // FIXME.SUGAR // SURE?
        /*
        >>> "%d" % 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
        '115792089237316195423570985008687907853269984665640564039457584007913129639935'
        >>> 115792089237316195423570985008687907853269984665640564039457584007913129639935 / 1024
        113078212145816597093331040047546785012958969400039613319782796882727665663L
        >>> "%x" % 113078212145816597093331040047546785012958969400039613319782796882727665663L
        '3fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff'
        >>>
        */

        // getdifficulty() == 2.384149979653205e-07
        // 0x1f3fffff == 0x003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.GetCompact()
        consensus.powLimit = uint256S("003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        // DEBUG - SUGAR
        // printf("\n*** BEGIN - DEBUG: MAIN\n");
        // uint32_t powLimitTOnBits = UintToArith256(consensus.powLimit).GetCompact();
        // printf("powLimitTOnBits = 0x%x\n", powLimitTOnBits);
        // printf("*** END - DEBUG\n");

        // SUGAR-HALVING
        // 17 hours = 17*60*60 = 61200 (was two weeks: 14×24×60×60 = 1209600)
        // available: 17*n hours or 17 days possible, because DigiShieldZEC uses n510 (17*n)
        consensus.nPowTargetTimespan = 61200;

        consensus.nPowAllowMinDifficultyBlocksAfterHeight = boost::none; // DigiShieldZEC
        consensus.nPowAveragingWindow = 510; // DigiShieldZEC // 2550/nPowTargetSpacing(5) = 510

        // DEBUG - SUGAR
        // printf("\n*** BEGIN - DEBUG: MAIN\n");
        // printf("nPowAveragingWindowRatio = %s\n", (maxUint/UintToArith256(consensus.powLimit)).ToString().c_str());
        // printf("nPowAveragingWindow = %ld\n", consensus.nPowAveragingWindow);
        // printf("*** END - DEBUG\n");

        assert(maxUint/UintToArith256(consensus.powLimit) == 1024); // DigiShieldZEC // 0x0000000000000000000000000000000000000000000000000000000000000400 == 1024
        assert(maxUint/UintToArith256(consensus.powLimit) >= consensus.nPowAveragingWindow); // DigiShieldZEC // true: 1024 >= 510

        consensus.nPowMaxAdjustDown = 32; // DigiShieldZEC // 32% adjustment down
        consensus.nPowMaxAdjustUp = 16; // DigiShieldZEC // 16% adjustment up

        // SUGAR-HALVING
        // 10*60/120 = 5 seconds block time
        // 120x faster than bitcoin
        consensus.nPowTargetSpacing = 5;

        consensus.fPowAllowMinDifficultyBlocks = false; // DigiShieldZEC
        consensus.fPowNoRetargeting = false; // DigiShieldZEC

        // SUGAR-HALVING
        // 75% of nMinerConfirmationWindow = 61200/5*0.75 = 9180 (was 1916 = 1209600/600*0.95+0.8)
        // 9180/510 = 18 cycles of DigiShieldZEC
        consensus.nRuleChangeActivationThreshold = 9180;

        // SUGAR-HALVING
        // nPowTargetTimespan/nPowTargetSpacing = 61200/5 = 12240 (was 2016 = 1209600/600)
        // 12240/510 = 24 cycles of DigiShieldZEC
        consensus.nMinerConfirmationWindow = 12240;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113 are always on
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // Deployment of SegWit (BIP141, BIP143, and BIP147) is always on
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00"); // FIXME.SUGAR

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("6068e545c100bdfa51beb3bf250656d1dd87030a56dc02b79b605a314e3681b5"); // genesis

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x9f;
        pchMessageStart[1] = 0xeb;
        pchMessageStart[2] = 0x4b;
        pchMessageStart[3] = 0x9d;
        nDefaultPort = 34230;
        nPruneAfterHeight = 100000;

        // nTime: date -d '2019-04-04 00:00:00 UTC' +%s = 1554336000 // MAINNET=+0 = 1554336000
        // genesisReward: pow(2,32) / COIN = 42.94967296 (was 50)
        genesis = CreateGenesisBlock(1554336000, 2645, 0x1f3fffff, 1, 42.94967296 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        // DEBUG - SUGAR
        // printf("***\n");
        // printf("genesis.GetPoWHash.MAIN = %s\n", genesis.GetPoWHash().ToString().c_str());
        // printf("genesis.GetHash.MAIN = %s\n", genesis.GetHash().ToString().c_str());
        // printf("genesis.hashMerkleRoot.MAIN %s\n",genesis.hashMerkleRoot.ToString().c_str());
        // printf("***\n");

        assert(genesis.GetPoWHash() == uint256S("002693799fdf94d1522a127e11903d5bc3a14edf866d00a5703e59159d644437")); // genesis
        assert(consensus.hashGenesisBlock == uint256S("6068e545c100bdfa51beb3bf250656d1dd87030a56dc02b79b605a314e3681b5")); // genesis
        assert(genesis.hashMerkleRoot == uint256S("4059da1dac0fe2761276837e64895d2ee55330558e58d29d7369499b31030102"));

        // TODO.SUGAR - seeder
        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they dont support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        // vSeeds.emplace_back("seed.bitcoin.sipa.be"); // Pieter Wuille, only supports x1, x5, x9, and xd
        // vSeeds.emplace_back("dnsseed.bluematt.me"); // Matt Corallo, only supports x9
        // vSeeds.emplace_back("dnsseed.bitcoin.dashjr.org"); // Luke Dashjr
        // vSeeds.emplace_back("seed.bitcoinstats.com"); // Christian Decker, supports x1 - xf
        // vSeeds.emplace_back("seed.bitcoin.jonasschnelli.ch"); // Jonas Schnelli, only supports x1, x5, x9, and xd
        // vSeeds.emplace_back("seed.btc.petertodd.org"); // Peter Todd, only supports x1, x5, x9, and xd

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,63);  // legacy: starting with S (upper)
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,125); // p2sh-segwit: starting with s (lower)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "sugar";   // bech32: starting with sugar1q

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = {
            {
                {0, uint256S("6068e545c100bdfa51beb3bf250656d1dd87030a56dc02b79b605a314e3681b5")}, // genesis
            }
        };

        // FIXME.SUGAR
        chainTxData = ChainTxData{
            // Data as of block 0000000000000000002d6cca6761c99b3c2e936f9a0e304b7c7651a993f461de (height 506081).
            0,  // * UNIX timestamp of last known number of transactions
            0,  // * total number of transactions between genesis and that timestamp
                //   (the tx=... number in the SetBestChain debug.log lines)
            0   // * estimated number of transactions per second after that timestamp
        };
    }
};

/**
 * Testnet (v4)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";

        // SUGAR-HALVING
        // BTC: (was 21000000)
        // around 2 years = pow(5,8)*32 = 390625*32 = 12500000
        // pow(5,8) * pow(2,32) / COIN = 390625 * 4294967296 / 100000000 = INTEGER (16777216.0)
        // pow(5,8) = 390625 is an integer cycle of pow(2,32) = 4294967296
        // 1st halving = 536870912*COIN = 12500000 * 42.94967296
        // Total Supply in COINs (in theory):	1073741824
        // Total Supply in COINs (in actual):	1073741823.87500000
        // Difference: 0.125
        consensus.nSubsidyHalvingInterval = 12500000;

        consensus.BIP16Height = 0;  // always on
        consensus.BIP34Height = 17;
        consensus.BIP34Hash = uint256S("");   // FIXME.SUGAR
        consensus.BIP65Height = 0; // always on
        consensus.BIP66Height = 0; // always on

        // GET powLimit by python // FIXME.SUGAR // SURE?
        /*
        >>> "%d" % 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
        '115792089237316195423570985008687907853269984665640564039457584007913129639935'
        >>> 115792089237316195423570985008687907853269984665640564039457584007913129639935 / 1024
        113078212145816597093331040047546785012958969400039613319782796882727665663L
        >>> "%x" % 113078212145816597093331040047546785012958969400039613319782796882727665663L
        '3fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff'
        >>>
        */

        // getdifficulty() == 2.384149979653205e-07
        // 0x1f3fffff == 0x003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff.GetCompact()
        consensus.powLimit = uint256S("003fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        // DEBUG - SUGAR
        // printf("\n*** BEGIN - DEBUG: TESTNET\n");
        // uint32_t powLimitTOnBits = UintToArith256(consensus.powLimit).GetCompact();
        // printf("powLimitTOnBits = 0x%x\n", powLimitTOnBits);
        // printf("*** END - DEBUG\n");

        // SUGAR-HALVING
        // 17 hours = 17*60*60 = 61200 (was two weeks: 14×24×60×60 = 1209600)
        // available: 17*n hours or 17 days possible, because DigiShieldZEC uses n510 (17*n)
        consensus.nPowTargetTimespan = 61200;

        consensus.nPowAllowMinDifficultyBlocksAfterHeight = boost::none; // DigiShieldZEC
        consensus.nPowAveragingWindow = 510; // 2550 / nPowTargetSpacing(5) = 510

        // DEBUG - SUGAR
        // printf("\n*** BEGIN - DEBUG: TESTNET\n");
        // printf("nPowAveragingWindowRatio = %s\n", (maxUint/UintToArith256(consensus.powLimit)).ToString().c_str());
        // printf("nPowAveragingWindow = %ld\n", consensus.nPowAveragingWindow);
        // printf("*** END - DEBUG\n");

        assert(maxUint/UintToArith256(consensus.powLimit) == 1024); // DigiShieldZEC // 0x0000000000000000000000000000000000000000000000000000000000000400 == 1024
        assert(maxUint/UintToArith256(consensus.powLimit) >= consensus.nPowAveragingWindow); // DigiShieldZEC // true: 1024 >= 510

        consensus.nPowMaxAdjustDown = 32; // DigiShieldZEC // 32% adjustment down
        consensus.nPowMaxAdjustUp = 16; // DigiShieldZEC // 16% adjustment up

        // SUGAR-HALVING
        // 10*60/120 = 5 seconds block time
        // 120x faster than bitcoin
        consensus.nPowTargetSpacing = 5;

        consensus.fPowAllowMinDifficultyBlocks = false; // DigiShieldZEC
        consensus.fPowNoRetargeting = false; // DigiShieldZEC

        // SUGAR-HALVING
        // 75% of nMinerConfirmationWindow = 61200/5*0.75 = 9180 (was 1916 = 1209600/600*0.95+0.8)
        // 9180/510 = 18 cycles of DigiShieldZEC
        consensus.nRuleChangeActivationThreshold = 9180;

        // SUGAR-HALVING
        // nPowTargetTimespan/nPowTargetSpacing = 61200/5 = 12240 (was 2016 = 1209600/600)
        // 12240/510 = 24 cycles of DigiShieldZEC
        consensus.nMinerConfirmationWindow = 12240;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113 are always on
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // Deployment of SegWit (BIP141, BIP143, and BIP147) is always on
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00"); // FIXME.SUGAR

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("e15ac61305c307e84db7906afef45a1acf1148b8c1f649645132ff66af7a760d"); // genesis

        pchMessageStart[0] = 0xb0;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x90;
        pchMessageStart[3] = 0x70;
        nDefaultPort = 44230;
        nPruneAfterHeight = 1000;

        // nTime: date -d '2019-04-04 00:00:00 UTC' +%s = 1554336000 // TESTNET=+1 = 1554336001
        // genesisReward: pow(2,32) / COIN = 42.94967296 (was 50)
        genesis = CreateGenesisBlock(1554336001, 201, 0x1f3fffff, 1, 42.94967296 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        // DEBUG - SUGAR
        // printf("***\n");
        // printf("genesis.GetPoWHash.TESTNET = %s\n", genesis.GetPoWHash().ToString().c_str());
        // printf("genesis.GetHash.TESTNET = %s\n", genesis.GetHash().ToString().c_str());
        // printf("genesis.hashMerkleRoot.TESTNET %s\n",genesis.hashMerkleRoot.ToString().c_str());
        // printf("***\n");

        assert(genesis.GetPoWHash() == uint256S("001f6ae059f121fee44269a7ff224c9672b4e7eeda95ab96356ccfdf39a70132")); // genesis
        assert(consensus.hashGenesisBlock == uint256S("e15ac61305c307e84db7906afef45a1acf1148b8c1f649645132ff66af7a760d")); // genesis
        assert(genesis.hashMerkleRoot == uint256S("4059da1dac0fe2761276837e64895d2ee55330558e58d29d7369499b31030102"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // TODO.SUGAR - seeder
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("seed-testnet.sugarchain.org");
        vSeeds.emplace_back("seed-testnet.cryptozeny.com");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,66);  // legacy: starting with T (upper)
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,128); // p2sh-segwit: starting with t (lower)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tugar";     // bech32: starting with tugar1q

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        checkpointData = {
            {
                {0, uint256S("e15ac61305c307e84db7906afef45a1acf1148b8c1f649645132ff66af7a760d")}, // genesis
            }
        };

        // FIXME.SUGAR
        chainTxData = ChainTxData{
            // Data as of block 000000000000033cfa3c975eb83ecf2bb4aaedf68e6d279f6ed2b427c64caff9 (height 1260526)
            0,
            0,
            0
        };

    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";

        // SUGAR-HALVING // (was same as BTC)
        consensus.nSubsidyHalvingInterval = 150;

        // DEBUG - SUGAR
        // printf("nSubsidyHalvingInterval = %d\n", consensus.nSubsidyHalvingInterval);

        consensus.BIP16Height = 0; // always enforce P2SH BIP16 on regtest
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)

        // GET powLimit by python // FIXME.SUGAR // SURE?
        /*
        >>> "%d" % 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
        '115792089237316195423570985008687907853269984665640564039457584007913129639935'
        >>> 115792089237316195423570985008687907853269984665640564039457584007913129639935 / 17
        6811299366900952671974763824040465167839410862684739061144563765171360567055L
        >>> "%x" % 6811299366900952671974763824040465167839410862684739061144563765171360567055L
        'f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f'
        >>>
        */

        // getdifficulty() == 3.958060781902051e-09
        // 0x200f0f0f == 0x0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f.GetCompact()
        consensus.powLimit = uint256S("0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f");

        // DEBUG - SUGAR
        // printf("\n*** BEGIN - DEBUG: REGTEST\n");
        // uint32_t powLimitTOnBits = UintToArith256(consensus.powLimit).GetCompact();
        // printf("powLimitTOnBits = 0x%x\n", powLimitTOnBits);
        // printf("*** END - DEBUG\n");

        // SUGAR-HALVING
        // 17 hours = 17*60*60 = 61200 (was two weeks: 14×24×60×60 = 1209600)
        // available: 17*n hours or 17 days possible, because DigiShieldZEC uses n510 (17*n)
        consensus.nPowTargetTimespan = 61200;

        consensus.nPowAllowMinDifficultyBlocksAfterHeight = boost::none; // DigiShieldZEC
        consensus.nPowAveragingWindow = 17; // DigiShieldZEC // 85/nPowTargetSpacing(5) = 17

        // DEBUG - SUGAR
        // printf("\n*** BEGIN - DEBUG: REGTEST\n");
        // printf("nPowAveragingWindowRatio = %s\n", (maxUint/UintToArith256(consensus.powLimit)).ToString().c_str());
        // printf("nPowAveragingWindow = %ld\n", consensus.nPowAveragingWindow);
        // printf("*** END - DEBUG\n");

        assert(maxUint/UintToArith256(consensus.powLimit) == 17); // DigiShieldZEC // 0x0000000000000000000000000000000000000000000000000000000000000011 == 17
        assert(maxUint/UintToArith256(consensus.powLimit) >= consensus.nPowAveragingWindow); // DigiShieldZEC // true: 17 >= 17

        consensus.nPowMaxAdjustDown = 0; // DigiShieldZEC // Turn off adjustment down
        consensus.nPowMaxAdjustUp = 0; // DigiShieldZEC // Turn off adjustment up

        // SUGAR-HALVING
        // 10*60/120 = 5 seconds block time
        // 120x faster than bitcoin
        consensus.nPowTargetSpacing = 5;

        consensus.fPowAllowMinDifficultyBlocks = true; // DigiShieldZEC
        consensus.fPowNoRetargeting = true; // DigiShieldZEC

        // SUGAR-HALVING // (was same as BTC)
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains

        // SUGAR-HALVING // (was same as BTC)
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("7c9807c6fa1ceae35770a6f41fd18a867dbd06c80af40446fc188ab50f0dbdcf"); // genesis

        pchMessageStart[0] = 0xaf;
        pchMessageStart[1] = 0xfb;
        pchMessageStart[2] = 0x5b;
        pchMessageStart[3] = 0xad;
        nDefaultPort = 45340;
        nPruneAfterHeight = 1000;

        // nTime: date -d '2019-04-04 00:00:00 UTC' +%s = 1554336000 // REGTEST=+2 = 1554336002
        // genesisReward: pow(2,32) / COIN = 42.94967296 (was 50)
        genesis = CreateGenesisBlock(1554336002, 11, 0x200f0f0f, 1, 42.94967296 * COIN);

        consensus.hashGenesisBlock = genesis.GetHash();

        // DEBUG - SUGAR
        // printf("\n*** BEGIN - DEBUG: REGTEST\n");
        // printf("genesis.GetPoWHash.REGTEST = %s\n", genesis.GetPoWHash().ToString().c_str());
        // printf("genesis.GetHash.REGTEST = %s\n", genesis.GetHash().ToString().c_str());
        // printf("genesis.hashMerkleRoot.REGTEST %s\n",genesis.hashMerkleRoot.ToString().c_str());
        // printf("*** END - DEBUG\n");

        assert(genesis.GetPoWHash() == uint256S("058dbd6e52e954425b873335939e5394228bf968c0071db2327579d6f0557c7a")); // genesis
        assert(consensus.hashGenesisBlock == uint256S("7c9807c6fa1ceae35770a6f41fd18a867dbd06c80af40446fc188ab50f0dbdcf")); // genesis
        assert(genesis.hashMerkleRoot == uint256S("4059da1dac0fe2761276837e64895d2ee55330558e58d29d7369499b31030102"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {
                {0, uint256S("7c9807c6fa1ceae35770a6f41fd18a867dbd06c80af40446fc188ab50f0dbdcf")}, // genesis
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,61);  // legacy: starting with R (upper)
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,123); // p2sh-segwit: starting with r (lower)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "rugar";    // bech32: starting with rugar1q
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
