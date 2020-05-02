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

    return pblock;
    printf("%s pblock=%s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str(), pblock->hashPrevBlock.ToString().c_str());
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
const std::shared_ptr<const CBlock> GoodBlock(const uint256& prev_hash)
{
    return FinalizeBlock(Block(prev_hash));
}

// construct an invalid block (but with a valid header)
const std::shared_ptr<const CBlock> BadBlock(const uint256& prev_hash)
{
    auto pblock = Block(prev_hash);

    CMutableTransaction coinbase_spend;
    coinbase_spend.vin.push_back(CTxIn(COutPoint(pblock->vtx[0]->GetHash(), 0), CScript(), 0));
    coinbase_spend.vout.push_back(pblock->vtx[0]->vout[0]);

    CTransactionRef tx = MakeTransactionRef(coinbase_spend);
    pblock->vtx.push_back(tx);

    auto ret = FinalizeBlock(pblock);
    return ret;
}

void BuildChain(const uint256& root, int height, const unsigned int fake_rate, const unsigned int max_size, std::vector<std::shared_ptr<const CBlock>>& blocks)
{
    printf("%s %lu/%u (%d) %s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str(), blocks.size(), max_size, height, root.ToString().c_str());
    // printf("%s blocks.size()=%lu/%u\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str(), blocks.size(), max_size);

    if (height <= 0 || blocks.size() >= max_size) return;

    bool gen_fakeheader = GetRand(100) < fake_rate;

    if (gen_fakeheader) {
        uint256 fake = uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        // BuildChain(fake, height - 1, invalid_rate, branch_rate, fake_rate, max_size, blocks);
        const std::shared_ptr<const CBlock> pblock = GoodBlock(root);
        blocks.push_back(pblock);
        BuildChain(fake, height - 1, fake_rate, max_size, blocks);
    } else {
        const std::shared_ptr<const CBlock> pblock = GoodBlock(root);
        blocks.push_back(pblock);
        BuildChain(pblock->GetHash(), height - 1, fake_rate, max_size, blocks);
    }
}

BOOST_AUTO_TEST_CASE(processnewblock_signals_ordering)
{
    unsigned int amount = 10;
    std::vector<std::shared_ptr<const CBlock>> blocks;
    while (blocks.size() < amount) {
        blocks.clear();
        // BuildChain(Params().GenesisBlock().GetHash(), 100, 15, 10, 500, blocks);
        int fake_percent = 50;
        BuildChain(Params().GenesisBlock().GetHash(), amount, fake_percent, amount, blocks);
    }
}

BOOST_AUTO_TEST_SUITE_END()
