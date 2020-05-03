// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <miner.h>
#include <pow.h>
#include <random.h>
#include <test/test_bitcoin.h>
#include <validation.h>
#include <validationinterface.h>

struct RegtestingSetup : public TestingSetup {
    RegtestingSetup() : TestingSetup(CBaseChainParams::REGTEST) {}
};

BOOST_FIXTURE_TEST_SUITE(validation_fakeheader_tests, RegtestingSetup)

struct TestSubscriber : public CValidationInterface {
    uint256 m_expected_tip;

    TestSubscriber(uint256 tip) : m_expected_tip(tip) {}

    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
    {
        BOOST_CHECK_EQUAL(m_expected_tip, pindexNew->GetBlockHash());
    }

    void BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex, const std::vector<CTransactionRef>& txnConflicted)
    {
        BOOST_CHECK_EQUAL(m_expected_tip, block->hashPrevBlock);
        BOOST_CHECK_EQUAL(m_expected_tip, pindex->pprev->GetBlockHash());

        m_expected_tip = block->GetHash();
    }

    void BlockDisconnected(const std::shared_ptr<const CBlock>& block)
    {
        BOOST_CHECK_EQUAL(m_expected_tip, block->GetHash());

        m_expected_tip = block->hashPrevBlock;
    }
};

std::shared_ptr<CBlock> Block(const uint256& prev_hash)
{
    static int i = 0;
    static uint64_t time = Params().GenesisBlock().nTime;

    CScript pubKey;
    pubKey << i++ << OP_TRUE;

    auto ptemplate = BlockAssembler(Params()).CreateNewBlock(pubKey, false);
    auto pblock = std::make_shared<CBlock>(ptemplate->block);
    pblock->hashPrevBlock = prev_hash;
    pblock->nTime = ++time;

    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    txCoinbase.vout.resize(1);
    txCoinbase.vin[0].scriptWitness.SetNull();
    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));

    // printf("%s pblock=%s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str(), pblock->hashPrevBlock.ToString().c_str());
    return pblock;
}

std::shared_ptr<CBlock> FinalizeBlock(std::shared_ptr<CBlock> pblock)
{
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

    while (!CheckProofOfWork(pblock->GetPoWHash(), pblock->nBits, Params().GetConsensus())) {
        ++(pblock->nNonce);
    }

    return pblock;
}

// construct a valid block
const std::shared_ptr<const CBlock> MakeBlock(const uint256& prev_hash)
{
    return FinalizeBlock(Block(prev_hash));
}

void BuildChain(const uint256& root, int height, const unsigned int fake_rate, const unsigned int max_size, std::vector<std::shared_ptr<const CBlock>>& blocks)
{
    printf("%s %lu/%u (%d) %s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str(), blocks.size(), max_size, height, root.ToString().c_str());

    if (height <= 0 || blocks.size() >= max_size) return;

    bool gen_fakeheader = GetRand(100) < fake_rate;

    if (gen_fakeheader) {
    // if (height % 3 == 0) {
        uint256 fake = uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // uint256 fake = uint256S("0x40813d2ba8cb96cfe5829ca949c6b020f898ddadc04c0fde599405eb4e6698e3"); // block 777
        // uint256 fake = InsecureRand256();
        const std::shared_ptr<const CBlock> pblock = MakeBlock(root);
        blocks.push_back(pblock);
        BuildChain(fake, height - 1, fake_rate, max_size, blocks);
    } else {
        const std::shared_ptr<const CBlock> pblock = MakeBlock(root);
        blocks.push_back(pblock);
        BuildChain(pblock->GetHash(), height - 1, fake_rate, max_size, blocks);
    }
}

void BuildChain2(const uint256& root, int height, const int limit, const unsigned int max_size, std::vector<std::shared_ptr<const CBlock>>& blocks)
{
    printf("%s %lu/%u (%d) %s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str(), blocks.size(), max_size, height, root.ToString().c_str());

    if (height <= 0 || blocks.size() >= max_size) return;

    // bool gen_fakeheader = GetRand(100) < fake_rate;

    // if (gen_fakeheader) {
    if (height <= limit) {
        uint256 fake = uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // uint256 fake = uint256S("0x40813d2ba8cb96cfe5829ca949c6b020f898ddadc04c0fde599405eb4e6698e3"); // block 777
        // uint256 fake = InsecureRand256();
        const std::shared_ptr<const CBlock> pblock = MakeBlock(root);
        blocks.push_back(pblock);
        BuildChain2(fake, height - 1, limit, max_size, blocks);
    } else {
        const std::shared_ptr<const CBlock> pblock = MakeBlock(root);
        blocks.push_back(pblock);
        BuildChain2(pblock->GetHash(), height - 1, limit, max_size, blocks);
    }
}

BOOST_AUTO_TEST_CASE(processnewblockheaders_fake_50)
{
    unsigned int amount = 10;
    std::vector<std::shared_ptr<const CBlock>> blocks;
    while (blocks.size() < amount) {
        blocks.clear();
        int fake_percent = 50;
        BuildChain(Params().GenesisBlock().GetHash(), amount, fake_percent, amount, blocks);
    }

    CValidationState state;
    std::vector<CBlockHeader> headers;
    std::transform(blocks.begin(), blocks.end(), std::back_inserter(headers), [](std::shared_ptr<const CBlock> b) { return b->GetBlockHeader(); });

    // Process all the headers so we understand the toplogy of the chain
    BOOST_CHECK_EQUAL(ProcessNewBlockHeaders(headers, state, Params()), false);
}

BOOST_AUTO_TEST_CASE(processnewblockheaders_fake_zero)
{
    unsigned int amount = 10;
    std::vector<std::shared_ptr<const CBlock>> blocks;
    while (blocks.size() < amount) {
        blocks.clear();
        // BuildChain(Params().GenesisBlock().GetHash(), 100, 15, 10, 500, blocks);
        int fake_percent = 0;
        BuildChain(Params().GenesisBlock().GetHash(), amount, fake_percent, amount, blocks);
    }

    CValidationState state;
    std::vector<CBlockHeader> headers;
    std::transform(blocks.begin(), blocks.end(), std::back_inserter(headers), [](std::shared_ptr<const CBlock> b) { return b->GetBlockHeader(); });

    // Process all the headers so we understand the toplogy of the chain
    BOOST_CHECK_EQUAL(ProcessNewBlockHeaders(headers, state, Params()), true);
}

BOOST_AUTO_TEST_CASE(processnewblockheaders_fake_50_newblock)
{
    int fake_amount = 3;
    unsigned int amount = 10;
    assert(amount-fake_amount <= 149); // no idea, but it should under 150

    std::vector<std::shared_ptr<const CBlock>> blocks;
    while (blocks.size() < amount) {
        blocks.clear();
        // BuildChain(Params().GenesisBlock().GetHash(), 100, 15, 10, 500, blocks);
        BuildChain2(Params().GenesisBlock().GetHash(), amount, fake_amount, amount, blocks);
    }

    bool ignored;
    CValidationState state;
    std::vector<CBlockHeader> headers;
    std::transform(blocks.begin(), blocks.end(), std::back_inserter(headers), [](std::shared_ptr<const CBlock> b) { return b->GetBlockHeader(); });

    // Process all the headers so we understand the toplogy of the chain
    unsigned int count = 0;
    for (auto block : blocks) {
        count++;
        if (count <= amount-fake_amount+1) {
            BOOST_CHECK_EQUAL(ProcessNewBlock(Params(), block, true, &ignored), true);
        } else {
            BOOST_CHECK_EQUAL(ProcessNewBlock(Params(), block, true, &ignored), false); // fake header should be false
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
