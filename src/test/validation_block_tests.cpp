// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <chainparams.h>
#include <checkpoints.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <miner.h>
#include <net.h>
#include <netbase.h>
#include <net_processing.h>
#include <pow.h>
#include <random.h>
#include <streams.h>
#include <clientversion.h>
#include <test/test_bitcoin.h>
#include <validation.h>
#include <validationinterface.h>
#include <ui_interface.h>

struct RegtestingSetup : public TestingSetup {
    RegtestingSetup() : TestingSetup(CBaseChainParams::REGTEST) {}
};

BOOST_FIXTURE_TEST_SUITE(validation_block_tests, RegtestingSetup)

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

void BuildChain(const uint256& root, int height, const unsigned int invalid_rate, const unsigned int branch_rate, const unsigned int max_size, std::vector<std::shared_ptr<const CBlock>>& blocks)
{
    if (height <= 0 || blocks.size() >= max_size) return;

    bool gen_invalid = GetRand(100) < invalid_rate;
    bool gen_fork = GetRand(100) < branch_rate;

    const std::shared_ptr<const CBlock> pblock = gen_invalid ? BadBlock(root) : GoodBlock(root);
    blocks.push_back(pblock);
    if (!gen_invalid) {
        BuildChain(pblock->GetHash(), height - 1, invalid_rate, branch_rate, max_size, blocks);
    }

    if (gen_fork) {
        blocks.push_back(GoodBlock(root));
        BuildChain(blocks.back()->GetHash(), height - 1, invalid_rate, branch_rate, max_size, blocks);
    }
}

BOOST_AUTO_TEST_CASE(processnewblock_signals_ordering)
{
    // build a large-ish chain that's likely to have some forks
    std::vector<std::shared_ptr<const CBlock>> blocks;
    while (blocks.size() < 50) {
        blocks.clear();
        BuildChain(Params().GenesisBlock().GetHash(), 100, 15, 10, 500, blocks);
    }

    bool ignored;
    CValidationState state;
    std::vector<CBlockHeader> headers;
    std::transform(blocks.begin(), blocks.end(), std::back_inserter(headers), [](std::shared_ptr<const CBlock> b) { return b->GetBlockHeader(); });

    // Process all the headers so we understand the toplogy of the chain
    BOOST_CHECK(ProcessNewBlockHeaders(headers, state, Params()));

    // Connect the genesis block and drain any outstanding events
    ProcessNewBlock(Params(), std::make_shared<CBlock>(Params().GenesisBlock()), true, &ignored);
    SyncWithValidationInterfaceQueue();

    // subscribe to events (this subscriber will validate event ordering)
    const CBlockIndex* initial_tip = nullptr;
    {
        LOCK(cs_main);
        initial_tip = chainActive.Tip();
    }
    TestSubscriber sub(initial_tip->GetBlockHash());
    RegisterValidationInterface(&sub);

    // create a bunch of threads that repeatedly process a block generated above at random
    // this will create parallelism and randomness inside validation - the ValidationInterface
    // will subscribe to events generated during block validation and assert on ordering invariance
    boost::thread_group threads;
    for (int i = 0; i < 10; i++) {
        threads.create_thread([&blocks]() {
            bool ignored;
            for (int i = 0; i < 1000; i++) {
                auto block = blocks[GetRand(blocks.size() - 1)];
                ProcessNewBlock(Params(), block, true, &ignored);
            }

            // to make sure that eventually we process the full chain - do it here
            for (auto block : blocks) {
                if (block->vtx.size() == 1) {
                    bool processed = ProcessNewBlock(Params(), block, true, &ignored);
                    assert(processed);
                }
            }
        });
    }

    threads.join_all();
    while (GetMainSignals().CallbacksPending() > 0) {
        MilliSleep(100);
    }

    UnregisterValidationInterface(&sub);

    BOOST_CHECK_EQUAL(sub.m_expected_tip, chainActive.Tip()->GetBlockHash());
}

BOOST_AUTO_TEST_CASE(processnewblock_checks_pow_during_ibd)
{
    // IsInitialBlockDownload() is process-global and latches false.  The full
    // unit-test binary may have left IBD in an earlier suite; the focused test
    // runs in a fresh process and exercises the fast-IBD path.
    if (!IsInitialBlockDownload()) {
        BOOST_TEST_MESSAGE("IBD already latched false; run this test case in isolation");
        return;
    }

    const auto check_invalid_pow = []() {
        auto pblock = Block(Params().GenesisBlock().GetHash());
        pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
        while (CheckProofOfWork(pblock->GetPoWHash(), pblock->nBits, Params().GetConsensus())) {
            ++pblock->nNonce;
        }
        bool new_block = false;
        BOOST_CHECK(!ProcessNewBlock(Params(), pblock, true, &new_block));
        BOOST_CHECK(!new_block);
    };

    // An untrusted block is rejected even when fast IBD is enabled.
    gArgs.ForceSetArg("-fast-ibd", "1");
    check_invalid_pow();

    // Disabling fast IBD retains full proof-of-work validation during IBD.
    gArgs.ForceSetArg("-fast-ibd", "0");
    check_invalid_pow();
    gArgs.ForceSetArg("-fast-ibd", "1");
}

BOOST_AUTO_TEST_CASE(fast_ibd_checkpoint_trust_is_ancestry_bound)
{
    uint256 hashes[4] = {uint256S("01"), uint256S("02"), uint256S("03"), uint256S("04")};
    CBlockIndex root;
    CBlockIndex trusted;
    CBlockIndex anchor;
    CBlockIndex fork;
    root.phashBlock = &hashes[0];
    trusted.phashBlock = &hashes[1];
    trusted.pprev = &root;
    trusted.nHeight = 1;
    anchor.phashBlock = &hashes[2];
    anchor.pprev = &trusted;
    anchor.nHeight = 2;
    fork.phashBlock = &hashes[3];
    fork.pprev = &root;
    fork.nHeight = 1;

    CCheckpointData checkpoints{{{2, hashes[2]}}};
    {
        LOCK(cs_main);
        mapBlockIndex.emplace(hashes[2], &anchor);
        BOOST_CHECK(Checkpoints::IsAncestorOfLastCheckpoint(&root, checkpoints));
        BOOST_CHECK(Checkpoints::IsAncestorOfLastCheckpoint(&trusted, checkpoints));
        BOOST_CHECK(Checkpoints::IsAncestorOfLastCheckpoint(&anchor, checkpoints));
        BOOST_CHECK(!Checkpoints::IsAncestorOfLastCheckpoint(&fork, checkpoints));
        mapBlockIndex.erase(hashes[2]);
    }

    BOOST_CHECK(Checkpoints::CheckBlock(1, hashes[3], checkpoints));
    BOOST_CHECK(Checkpoints::CheckBlock(2, hashes[2], checkpoints));
    BOOST_CHECK(!Checkpoints::CheckBlock(2, hashes[3], checkpoints));
}


namespace {
struct FastIBDOptions {
    const std::string old_fast{gArgs.GetArg("-fast-ibd", "1")};
    const bool old_checkpoints{fCheckpointsEnabled};
    FastIBDOptions() { gArgs.ForceSetArg("-fast-ibd", "1"); fCheckpointsEnabled = true; }
    ~FastIBDOptions() { gArgs.ForceSetArg("-fast-ibd", old_fast); fCheckpointsEnabled = old_checkpoints; }
};

class CheckpointTestParams : public CChainParams {
public:
    explicit CheckpointTestParams(const CCheckpointData& checkpoints) : CChainParams(Params()) { checkpointData = checkpoints; }
};

std::vector<CBlockHeader> HeaderChain(size_t count)
{
    std::vector<CBlockHeader> headers;
    uint256 prev = Params().GenesisBlock().GetHash();
    for (size_t i = 0; i < count; ++i) {
        CBlockHeader header;
        header.nVersion = VERSIONBITS_TOP_BITS;
        header.hashPrevBlock = prev;
        header.nBits = Params().GenesisBlock().nBits;
        header.nTime = Params().GenesisBlock().nTime + i + 1;
        prev = header.GetHash();
        headers.push_back(header);
    }
    return headers;
}

void MakeInvalidPoW(CBlockHeader& header)
{
    while (CheckProofOfWork(header.GetPoWHash(), header.nBits, Params().GetConsensus())) ++header.nNonce;
    header.cache_init = false;
}
}

BOOST_AUTO_TEST_CASE(fast_ibd_rejects_unproven_headers_and_disk_blocks)
{
    FastIBDOptions options;
    auto block = Block(Params().GenesisBlock().GetHash());
    block->hashMerkleRoot = BlockMerkleRoot(*block);
    MakeInvalidPoW(*block);
    const auto* best_before = pindexBestHeader;
    const auto size_before = mapBlockIndex.size();
    for (bool fast : {true, false}) {
        gArgs.ForceSetArg("-fast-ibd", fast ? "1" : "0");
        for (bool checkpoints : {true, false}) {
            fCheckpointsEnabled = checkpoints;
            CValidationState state;
            BOOST_CHECK(!ProcessNewBlockHeaders({block->GetBlockHeader()}, state, Params()));
            BOOST_CHECK_EQUAL(state.GetRejectReason(), "high-hash");
            CValidationState block_state;
            BOOST_CHECK(!CheckBlock(*block, block_state, Params().GetConsensus()));
            BOOST_CHECK_EQUAL(mapBlockIndex.size(), size_before);
            BOOST_CHECK(pindexBestHeader == best_before);
        }
    }
    gArgs.ForceSetArg("-fast-ibd", "1");
    fCheckpointsEnabled = true;
    CDiskBlockPos pos(99, 0);
    {
        CAutoFile file(OpenBlockFile(pos), SER_DISK, CLIENT_VERSION);
        BOOST_REQUIRE(!file.IsNull());
        file << *block;
    }
    CBlock disk_block;
    BOOST_CHECK(!ReadBlockFromDisk(disk_block, pos, Params().GetConsensus()));
}

BOOST_AUTO_TEST_CASE(checkpoint_presync_authenticates_before_indexing)
{
    FastIBDOptions options;
    auto headers = HeaderChain(2003);
    const CCheckpointData checkpoints{{{510, headers[509].GetHash()}, {2003, headers.back().GetHash()}}};
    CheckpointTestParams params(checkpoints);
    Checkpoints::HeaderSync sync(0, Params().GenesisBlock().GetHash(), checkpoints);
    BOOST_CHECK_EQUAL(sync.StartHeight(), 0);
    BOOST_CHECK_EQUAL(sync.StopHeight(), checkpoints.mapCheckpoints.rbegin()->first);
    std::vector<CBlockHeader> authenticated;
    const auto size_before = mapBlockIndex.size();
    const auto* best_before = pindexBestHeader;
    BOOST_REQUIRE(sync.Process({headers.begin(), headers.begin() + 2000}, authenticated));
    BOOST_CHECK(authenticated.empty());
    BOOST_CHECK_EQUAL(sync.CommitmentCount(), 1U);
    BOOST_CHECK(!sync.Authenticates(headers.front().GetHash(), checkpoints));
    BOOST_REQUIRE(sync.Process({headers.begin() + 2000, headers.end()}, authenticated));
    BOOST_CHECK(authenticated.empty());
    BOOST_CHECK(sync.Replaying());
    BOOST_CHECK_EQUAL(sync.Height(), 0);
    BOOST_CHECK_EQUAL(mapBlockIndex.size(), size_before);
    BOOST_CHECK(pindexBestHeader == best_before);

    // Different packet boundaries must not change the committed chunk size.
    BOOST_REQUIRE(sync.Process({headers.begin(), headers.begin() + 1001}, authenticated));
    BOOST_CHECK(authenticated.empty());
    BOOST_REQUIRE(sync.Process({headers.begin() + 1001, headers.begin() + 2000}, authenticated));
    BOOST_REQUIRE_EQUAL(authenticated.size(), 2000U);
    BOOST_CHECK(sync.Authenticates(headers[0].GetHash(), checkpoints));
    const CCheckpointData other_checkpoints{{{2003, uint256S("1234")}}};
    BOOST_CHECK(!sync.Authenticates(headers[0].GetHash(), other_checkpoints));
    CValidationState state;
    BOOST_REQUIRE(ProcessNewBlockHeaders(authenticated, state, params, nullptr, nullptr, &sync));
    BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 2000);
    BOOST_CHECK(pindexBestHeader->nStatus & BLOCK_CHECKPOINT_CHECKED);
    BOOST_CHECK(!(pindexBestHeader->nStatus & BLOCK_POW_CHECKED));
    BOOST_REQUIRE(sync.Process({headers.begin() + 2000, headers.end()}, authenticated));
    BOOST_CHECK(sync.Complete());
    BOOST_REQUIRE(ProcessNewBlockHeaders(authenticated, state, params, nullptr, nullptr, &sync));
    BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 2003);

    // A post-checkpoint child cannot claim work without actually proving it.
    CBlockHeader child = headers.back();
    child.hashPrevBlock = child.GetHash();
    child.nTime++;
    MakeInvalidPoW(child);
    CValidationState invalid;
    BOOST_CHECK(!ProcessNewBlockHeaders({child}, invalid, params, nullptr, nullptr, &sync));
    BOOST_CHECK_EQUAL(invalid.GetRejectReason(), "high-hash");
    BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 2003);

    // The evidence is intentionally persistent across restart mid-replay.
    CDiskBlockIndex disk(pindexBestHeader), restored;
    CDataStream stream(SER_DISK, CLIENT_VERSION);
    stream << disk;
    stream >> restored;
    BOOST_CHECK(restored.nStatus & BLOCK_CHECKPOINT_CHECKED);
    BOOST_CHECK(!(restored.nStatus & BLOCK_POW_CHECKED));
}

BOOST_AUTO_TEST_CASE(checkpoint_presync_rejects_forks_and_replay_equivocation)
{
    const auto headers = HeaderChain(2003);
    const CCheckpointData checkpoints{{{510, headers[509].GetHash()}, {2003, headers.back().GetHash()}}};
    std::vector<CBlockHeader> out;
    // A fake segment cannot connect to the next existing checkpoint.
    Checkpoints::HeaderSync fake(0, Params().GenesisBlock().GetHash(), checkpoints);
    auto fork = headers;
    ++fork[509].nNonce;
    BOOST_CHECK(!fake.Process({fork.begin(), fork.begin() + 510}, out));
    BOOST_CHECK(out.empty());
    BOOST_CHECK_EQUAL(fake.CommitmentCount(), 0U);
    BOOST_CHECK(!fake.Process({headers.begin(), headers.begin() + 510}, out));

    // Change the first replay header and re-link the whole chunk. The fixed
    // first-pass commitment must still reject it, even without an interior CP.
    const CCheckpointData endpoint_only{{{2003, headers.back().GetHash()}}};
    Checkpoints::HeaderSync equivocation(0, Params().GenesisBlock().GetHash(), endpoint_only);
    BOOST_REQUIRE(equivocation.Process({headers.begin(), headers.begin() + 2000}, out));
    BOOST_REQUIRE(equivocation.Process({headers.begin() + 2000, headers.end()}, out));
    fork = headers;
    ++fork[0].nNonce;
    for (size_t i = 1; i < 2000; ++i) fork[i].hashPrevBlock = fork[i-1].GetHash();
    BOOST_CHECK(!equivocation.Process({fork.begin(), fork.begin() + 2000}, out));
    BOOST_CHECK(out.empty());
    BOOST_CHECK(!equivocation.Authenticates(fork[0].GetHash(), endpoint_only));

    Checkpoints::HeaderSync oversized(0, Params().GenesisBlock().GetHash(), endpoint_only);
    BOOST_CHECK(!oversized.Process(headers, out));
    BOOST_CHECK_EQUAL(oversized.CommitmentCount(), 0U);
    BOOST_CHECK_EQUAL(oversized.Height(), 0);
}

BOOST_AUTO_TEST_CASE(checkpoint_replay_resume_requires_accepted_commitments)
{
    FastIBDOptions options;
    const auto headers = HeaderChain(4003);
    const CCheckpointData checkpoints{{{4003, headers.back().GetHash()}}};
    CheckpointTestParams params(checkpoints);
    Checkpoints::HeaderSync sync(0, Params().GenesisBlock().GetHash(), checkpoints);
    std::vector<CBlockHeader> out;
    LOCK(cs_main);
    BOOST_CHECK(!sync.Resume(checkpoints)); // Unauthenticated presync cannot authorize replay.
    BOOST_REQUIRE(sync.Process({headers.begin(), headers.begin() + 2000}, out));
    BOOST_REQUIRE(sync.Process({headers.begin() + 2000, headers.begin() + 4000}, out));
    BOOST_REQUIRE(sync.Process({headers.begin() + 4000, headers.end()}, out));
    const Checkpoints::HeaderSync snapshot(sync);
    BOOST_CHECK(!snapshot.Resume(CCheckpointData{{{4003, uint256S("1234")}}}));
    BOOST_REQUIRE(sync.Process({headers.begin(), headers.begin() + 2000}, out));
    // Authentication alone is insufficient to skip contextual index admission.
    BOOST_REQUIRE(snapshot.Resume(checkpoints));
    BOOST_CHECK_EQUAL(snapshot.Resume(checkpoints)->Height(), 0);
    CValidationState state;
    BOOST_REQUIRE(ProcessNewBlockHeaders(out, state, params, nullptr, nullptr, &sync));
    auto resumed = snapshot.Resume(checkpoints);
    BOOST_REQUIRE(resumed);
    BOOST_CHECK_EQUAL(resumed->Height(), 2000);
    BOOST_CHECK(resumed->NextHash() == headers[1999].GetHash());
    BOOST_CHECK(!resumed->Authenticates(headers[0].GetHash(), checkpoints));

    auto* accepted = mapBlockIndex.at(headers[1999].GetHash());
    const auto status = accepted->nStatus;
    accepted->nStatus &= ~BLOCK_CHECKPOINT_CHECKED;
    BOOST_CHECK_EQUAL(snapshot.Resume(checkpoints)->Height(), 0);
    accepted->nStatus = status | BLOCK_FAILED_VALID;
    BOOST_CHECK_EQUAL(snapshot.Resume(checkpoints)->Height(), 0);
    accepted->nStatus = status;

    BOOST_REQUIRE(resumed->Process({headers.begin() + 2000, headers.begin() + 4000}, out));
    BOOST_REQUIRE(ProcessNewBlockHeaders(out, state, params, nullptr, nullptr, resumed.get()));
    BOOST_REQUIRE(resumed->Process({headers.begin() + 4000, headers.end()}, out));
    BOOST_REQUIRE(ProcessNewBlockHeaders(out, state, params, nullptr, nullptr, resumed.get()));
    BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 4003);
    BOOST_CHECK(!snapshot.Resume(checkpoints)); // No historical replay after completion.
}

BOOST_AUTO_TEST_CASE(checkpoint_proof_never_disables_other_validation_or_full_mode)
{
    FastIBDOptions options;
    auto block = Block(Params().GenesisBlock().GetHash());
    block->hashMerkleRoot = BlockMerkleRoot(*block);
    MakeInvalidPoW(*block);
    const CCheckpointData checkpoints{{{1, block->GetHash()}}};
    CheckpointTestParams params(checkpoints);
    Checkpoints::HeaderSync sync(0, Params().GenesisBlock().GetHash(), checkpoints);
    std::vector<CBlockHeader> authenticated;
    BOOST_REQUIRE(sync.Process({block->GetBlockHeader()}, authenticated));
    BOOST_REQUIRE(sync.Process({block->GetBlockHeader()}, authenticated));

    // A test checkpoint deliberately commits invalid PoW: full mode must
    // independently reject it. This models a mistaken/malicious trust anchor.
    gArgs.ForceSetArg("-fast-ibd", "0");
    CValidationState full;
    BOOST_CHECK(!ProcessNewBlockHeaders(authenticated, full, params, nullptr, nullptr, &sync));
    BOOST_CHECK_EQUAL(full.GetRejectReason(), "high-hash");
    gArgs.ForceSetArg("-fast-ibd", "1");
    fCheckpointsEnabled = false;
    CValidationState disabled;
    BOOST_CHECK(!ProcessNewBlockHeaders(authenticated, disabled, params, nullptr, nullptr, &sync));
    fCheckpointsEnabled = true;
    for (auto* flag : {&fReindex, &fImporting}) {
        *flag = true;
        CValidationState importing;
        BOOST_CHECK(!ProcessNewBlockHeaders(authenticated, importing, params, nullptr, nullptr, &sync));
        *flag = false;
    }
    CValidationState trusted;
    BOOST_REQUIRE(ProcessNewBlockHeaders(authenticated, trusted, params, nullptr, nullptr, &sync));
    CValidationState checked_in_fast_mode;
    BOOST_REQUIRE(CheckBlock(*block, checked_in_fast_mode, params.GetConsensus()));
    BOOST_REQUIRE(block->fChecked);
    gArgs.ForceSetArg("-fast-ibd", "0");
    CValidationState checked_in_full_mode;
    BOOST_CHECK(!CheckBlock(*block, checked_in_full_mode, params.GetConsensus()));
    BOOST_CHECK_EQUAL(checked_in_full_mode.GetRejectReason(), "high-hash");
    gArgs.ForceSetArg("-fast-ibd", "1");
    block->fChecked = false;
    CValidationState body;
    block->vtx.clear(); // header authentication cannot authorize an invalid body
    BOOST_CHECK(!CheckBlock(*block, body, params.GetConsensus()));
    BOOST_CHECK_EQUAL(body.GetRejectReason(), "bad-txnmrklroot");
    gArgs.ForceSetArg("-fast-ibd", "0");
    CValidationState duplicate;
    BOOST_CHECK(!ProcessNewBlockHeaders(authenticated, duplicate, params));
    BOOST_CHECK_EQUAL(duplicate.GetRejectReason(), "high-hash");
}

BOOST_AUTO_TEST_CASE(header_pow_evidence_is_exact_and_parallel_validation_is_ordered)
{
    FastIBDOptions options;
    boost::thread_group workers;
    workers.create_thread(&ThreadHeaderPoWCheck);
    workers.create_thread(&ThreadHeaderPoWCheck);
    auto headers = HeaderChain(3);
    for (size_t i = 0; i < headers.size(); ++i) {
        if (i) headers[i].hashPrevBlock = headers[i-1].GetHash();
        while (!CheckProofOfWork(headers[i].GetPoWHash(), headers[i].nBits, Params().GetConsensus())) ++headers[i].nNonce;
    }
    MakeInvalidPoW(headers[1]);
    headers[2].hashPrevBlock = headers[1].GetHash();
    CBlockHeader first_invalid;
    CValidationState state;
    BOOST_CHECK(!ProcessNewBlockHeaders(headers, state, Params(), nullptr, &first_invalid));
    BOOST_CHECK(first_invalid.GetHash() == headers[1].GetHash());
    BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 1);
    BOOST_CHECK(pindexBestHeader->nStatus & BLOCK_POW_CHECKED);
    BOOST_CHECK(!mapBlockIndex.count(headers[1].GetHash()));
    BOOST_CHECK(!mapBlockIndex.count(headers[2].GetHash()));
    workers.interrupt_all();
    workers.join_all();

    // Repeated delivery reuses actual proof; it must not calculate Yespower in
    // the new object. Mutation cannot reuse the original header's evidence.
    CBlockHeader duplicate = headers[0];
    duplicate.cache_init = false;
    CValidationState repeated;
    std::vector<CBlockHeader> repeated_headers{duplicate};
    BOOST_REQUIRE(ProcessNewBlockHeaders(repeated_headers, repeated, Params()));
    BOOST_CHECK(!repeated_headers[0].cache_init);
    MakeInvalidPoW(duplicate);
    CValidationState changed;
    BOOST_CHECK(!ProcessNewBlockHeaders({duplicate}, changed, Params()));
    BOOST_CHECK_EQUAL(changed.GetRejectReason(), "high-hash");
}


BOOST_AUTO_TEST_CASE(checkpoint_authentication_through_p2p_headers_messages)
{
    FastIBDOptions options;
    struct Progress { int start, height, target; bool replay; };
    std::vector<Progress> progress;
    boost::signals2::scoped_connection progress_connection(uiInterface.NotifyCheckpointHeaderProgress.connect(
        [&](int start, int height, int target, bool replay) { progress.push_back({start, height, target, replay}); }));
    const auto headers = HeaderChain(8003);
    auto& checkpoints = const_cast<CCheckpointData&>(Params().Checkpoints());
    struct RestoreCheckpoints {
        CCheckpointData& ref;
        CCheckpointData original;
        ~RestoreCheckpoints() { ref = original; }
    } restore{checkpoints, checkpoints};
    checkpoints = {{{510, headers[509].GetHash()}, {8003, headers.back().GetHash()}}};
    CConnman::Options conn_options;
    conn_options.nSendBufferMaxSize = 4 * 1024 * 1024;
    conn_options.nReceiveFloodSize = 4 * 1024 * 1024;
    connman->Init(conn_options);
    CService service;
    BOOST_REQUIRE(Lookup("250.1.1.1", service, 18444, false));
    CAddress addr(service, NODE_NETWORK);
    const int64_t start_time = GetTime();
    SetMockTime(start_time);
    struct RestoreTime { ~RestoreTime() { SetMockTime(0); } } restore_time;
    // Bad presync, stalled presync, absolute presync timeout, healthy long
    // replay, equivocating replacement, partial-packet peer loss, completion.
    for (int scenario : {0, 1, 6, 2, 3, 4, 5}) {
        if (!progress.empty()) BOOST_CHECK_EQUAL(progress.back().target, 0);
        progress.clear();
        CNode peer(12345 + scenario, ServiceFlags(NODE_NETWORK | NODE_WITNESS), 0, INVALID_SOCKET, addr, 0, 0, CAddress(), "", false);
        peer.SetSendVersion(PROTOCOL_VERSION);
        peerLogic->InitializeNode(&peer);
        struct FinalizePeer {
            PeerLogicValidation* logic;
            NodeId id;
            ~FinalizePeer() { bool dummy; logic->FinalizeNode(id, dummy); }
        } finalize{peerLogic.get(), peer.GetId()};
        peer.nVersion = PROTOCOL_VERSION;
        peer.fSuccessfullyConnected = true;
        std::atomic<bool> interrupt{false};
        {
            LOCK(peer.cs_sendProcessing);
            peerLogic->SendMessages(&peer, interrupt);
        }
        BOOST_REQUIRE_EQUAL(progress.size(), 1U);
        BOOST_CHECK_EQUAL(progress.back().start, 0);
        BOOST_CHECK_EQUAL(progress.back().height, scenario >= 3 && scenario != 6 ? 4000 : 0);
        BOOST_CHECK_EQUAL(progress.back().target, 8003);
        BOOST_CHECK_EQUAL(progress.back().replay, scenario >= 3 && scenario != 6);
        const auto receive = [&](const std::vector<CBlockHeader>& packet, CNode* sender = nullptr) {
            CNode& target = sender ? *sender : peer;
            CDataStream payload(SER_NETWORK, PROTOCOL_VERSION);
            WriteCompactSize(payload, packet.size());
            for (const auto& header : packet) { payload << header; WriteCompactSize(payload, 0); }
            CNetMessage message(Params().MessageStart(), SER_NETWORK, PROTOCOL_VERSION);
            message.hdr = CMessageHeader(Params().MessageStart(), NetMsgType::HEADERS, payload.size());
            message.in_data = true;
            message.nTime = GetTimeMicros();
            message.readData(payload.data(), payload.size());
            const auto& hash = message.GetMessageHash();
            std::copy(hash.begin(), hash.begin() + CMessageHeader::CHECKSUM_SIZE, message.hdr.pchChecksum);
            target.nProcessQueueSize += payload.size() + CMessageHeader::HEADER_SIZE;
            target.vProcessMsg.push_back(std::move(message));
            peerLogic->ProcessMessages(&target, interrupt);
        };
        if (scenario == 1) {
            SetMockTime(GetTime() + 61);
            { LOCK(peer.cs_sendProcessing); peerLogic->SendMessages(&peer, interrupt); }
            SetMockTime(start_time);
            BOOST_CHECK(peer.fDisconnect);
            BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
            continue;
        }
        if (scenario == 6) {
            SetMockTime(start_time + 4 * 60 * 60 + 1);
            receive({headers.begin(), headers.begin() + 2000});
            { LOCK(peer.cs_sendProcessing); peerLogic->SendMessages(&peer, interrupt); }
            BOOST_CHECK(peer.fDisconnect); // Progress cannot extend unauthenticated presync.
            BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
            SetMockTime(start_time);
            continue;
        }
        if (scenario >= 3) {
            // The replacement request must start at the last accepted chunk,
            // not checkpoint 510, genesis, or the previous partial packet.
            bool found_request = false;
            for (size_t i = 0; i + 1 < peer.vSendMsg.size(); ++i) {
                if (peer.vSendMsg[i].size() != CMessageHeader::HEADER_SIZE) continue;
                CDataStream envelope(peer.vSendMsg[i], SER_NETWORK, PROTOCOL_VERSION);
                CMessageHeader header(Params().MessageStart());
                envelope >> header;
                if (header.GetCommand() != NetMsgType::GETHEADERS) continue;
                CDataStream payload(peer.vSendMsg[i + 1], SER_NETWORK, PROTOCOL_VERSION);
                CBlockLocator locator;
                uint256 stop;
                payload >> locator >> stop;
                BOOST_REQUIRE(!locator.vHave.empty());
                BOOST_CHECK(locator.vHave.front() == headers[3999].GetHash());
                BOOST_CHECK(stop == headers.back().GetHash());
                found_request = true;
            }
            BOOST_REQUIRE(found_request);
            if (scenario == 3) {
                auto fork = std::vector<CBlockHeader>(headers.begin() + 4000, headers.begin() + 6000);
                ++fork[0].nNonce;
                for (size_t i = 1; i < fork.size(); ++i) fork[i].hashPrevBlock = fork[i - 1].GetHash();
                receive(fork);
                BOOST_CHECK(peer.fDisconnect);
                BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 4000);
                BOOST_CHECK_EQUAL(mapBlockIndex.size(), 4001U);
                continue;
            }
            if (scenario == 4) {
                receive({headers.begin() + 4000, headers.begin() + 4017});
                BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 4000);
                continue; // FinalizeNode with a still-unverified partial chunk.
            }
            receive({headers.begin() + 4000, headers.begin() + 6000});
            BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 6000); // No second presync.
            receive({headers.begin() + 6000, headers.begin() + 8000});
            receive({headers.begin() + 8000, headers.end()});
            BOOST_CHECK_EQUAL(progress.back().target, 0); // Replay completion clears the UI.
            BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 8003);
            BOOST_CHECK(!peer.fDisconnect);
            CBlockHeader invalid = headers.back();
            invalid.hashPrevBlock = invalid.GetHash();
            invalid.nTime++;
            MakeInvalidPoW(invalid);
            receive({invalid});
            BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 8003);
            BOOST_CHECK(!mapBlockIndex.count(invalid.GetHash()));
            continue;
        }
        if (scenario == 0) {
            auto fork = headers;
            ++fork[509].nNonce;
            receive({fork.begin(), fork.begin() + 2000});
            BOOST_CHECK(peer.fDisconnect);
            BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
            continue; // RAII finalization releases the sync slot for the next peer.
        }
        {
            CNode unsolicited(12350, ServiceFlags(NODE_NETWORK | NODE_WITNESS), 0, INVALID_SOCKET, addr, 0, 0, CAddress(), "", false);
            unsolicited.SetSendVersion(PROTOCOL_VERSION);
            peerLogic->InitializeNode(&unsolicited);
            FinalizePeer finalize_unsolicited{peerLogic.get(), unsolicited.GetId()};
            unsolicited.nVersion = PROTOCOL_VERSION;
            unsolicited.fSuccessfullyConnected = true;
            receive({headers.begin(), headers.begin() + 2000}, &unsolicited);
            BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
        }
        for (size_t i = 0; i < headers.size(); i += 2000) {
            receive({headers.begin() + i, headers.begin() + std::min(i + 2000, headers.size())});
            BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
            BOOST_CHECK(!peer.fDisconnect);
        }
        // No per-packet GUI updates below the 100,000-height boundary;
        // the presync -> replay transition is nevertheless immediate.
        BOOST_REQUIRE_EQUAL(progress.size(), 2U);
        BOOST_CHECK(progress.back().replay);
        BOOST_CHECK_EQUAL(progress.back().height, 0);
        BOOST_CHECK_EQUAL(progress.back().target, 8003);
        receive({headers.begin(), headers.begin() + 2000});
        BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 2000);
        // Reproduce the observed trigger: useful replay crosses the original
        // four-hour session deadline. It must not disconnect a progressing peer.
        SetMockTime(start_time + 4 * 60 * 60 + 1);
        receive({headers.begin() + 2000, headers.begin() + 4000});
        { LOCK(peer.cs_sendProcessing); peerLogic->SendMessages(&peer, interrupt); }
        BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 4000);
        BOOST_CHECK(!peer.fDisconnect);
        receive({headers.begin() + 4000, headers.begin() + 4023});
        BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 4000);
        SetMockTime(GetTime() + 61);
        { LOCK(peer.cs_sendProcessing); peerLogic->SendMessages(&peer, interrupt); }
        BOOST_CHECK(peer.fDisconnect); // Replay still cannot stall indefinitely.
    }
}

// Real HEADERS parsing and peer lifecycle, with a tiny synthetic checkpoint.
// No checkpoint_sync internals or claimed progress heights are assigned.
BOOST_AUTO_TEST_CASE(checkpoint_presync_peer_takeover)
{
    FastIBDOptions options;
    const auto headers = HeaderChain(4003);
    auto& checkpoints = const_cast<CCheckpointData&>(Params().Checkpoints());
    struct Restore {
        CCheckpointData& ref;
        CCheckpointData original;
        ~Restore() { ref = original; SetMockTime(0); }
    } restore{checkpoints, checkpoints};
    checkpoints = {{{4003, headers.back().GetHash()}}};
    CConnman::Options conn_options;
    conn_options.nSendBufferMaxSize = conn_options.nReceiveFloodSize = 4 * 1024 * 1024;
    connman->Init(conn_options);
    CService service;
    BOOST_REQUIRE(Lookup("250.1.1.2", service, 18444, false));
    CAddress addr(service, NODE_NETWORK);
    std::atomic<bool> interrupt{false};
    int height = -1;
    bool replay = false;
    boost::signals2::scoped_connection progress(uiInterface.NotifyCheckpointHeaderProgress.connect(
        [&](int, int h, int, bool r) { height = h; replay = r; }));
    const auto send = [&](CNode& peer) {
        LOCK(peer.cs_sendProcessing);
        peerLogic->SendMessages(&peer, interrupt);
    };
    const auto receive = [&](CNode& peer, const std::vector<CBlockHeader>& packet) {
        CDataStream payload(SER_NETWORK, PROTOCOL_VERSION);
        WriteCompactSize(payload, packet.size());
        for (const auto& header : packet) { payload << header; WriteCompactSize(payload, 0); }
        CNetMessage message(Params().MessageStart(), SER_NETWORK, PROTOCOL_VERSION);
        message.hdr = CMessageHeader(Params().MessageStart(), NetMsgType::HEADERS, payload.size());
        message.in_data = true;
        message.nTime = GetTimeMicros();
        message.readData(payload.data(), payload.size());
        const auto& hash = message.GetMessageHash();
        std::copy(hash.begin(), hash.begin() + CMessageHeader::CHECKSUM_SIZE, message.hdr.pchChecksum);
        peer.nProcessQueueSize += payload.size() + CMessageHeader::HEADER_SIZE;
        peer.vProcessMsg.push_back(std::move(message));
        peerLogic->ProcessMessages(&peer, interrupt);
    };
    // Disconnect and inactivity timeout before authentication, followed by
    // replacement peers, through InitializeNode/FinalizeNode/SendMessages.
    const int64_t start_time = Params().GenesisBlock().nTime + 100000;
    for (int scenario : {0, 1, 2, 3, 4, 6, 7, 5}) {
        checkpoints = {{{4003, headers.back().GetHash()}}};
        SetMockTime(start_time);
        for (int replacement = 0; replacement < 2; ++replacement) {
            if (replacement && scenario == 3) SetMockTime(start_time + 4 * 60 * 60 + 1);
            if (replacement && scenario == 4) checkpoints = {{{4002, headers[4001].GetHash()}}};
            CNode peer(13000 + scenario * 2 + replacement,
                ServiceFlags(NODE_NETWORK | NODE_WITNESS), 0, INVALID_SOCKET, addr, 0, 0, CAddress(), "", false);
            peer.SetSendVersion(PROTOCOL_VERSION);
            peerLogic->InitializeNode(&peer);
            struct Finalize {
                PeerLogicValidation* logic;
                NodeId id;
                ~Finalize() { bool dummy; logic->FinalizeNode(id, dummy); }
            } finalize{peerLogic.get(), peer.GetId()};
            peer.nVersion = PROTOCOL_VERSION;
            peer.fSuccessfullyConnected = true;
            send(peer);
            const bool continued = replacement && (scenario < 2 || scenario >= 5);
            BOOST_CHECK_EQUAL(height, continued ? 2017 : 0);
            BOOST_CHECK(!replay);
            // Verify the actual wire request, not just the UI progress height.
            bool requested = false;
            for (size_t i = 0; i + 1 < peer.vSendMsg.size(); ++i) {
                if (peer.vSendMsg[i].size() != CMessageHeader::HEADER_SIZE) continue;
                CDataStream envelope(peer.vSendMsg[i], SER_NETWORK, PROTOCOL_VERSION);
                CMessageHeader header(Params().MessageStart());
                envelope >> header;
                if (header.GetCommand() != NetMsgType::GETHEADERS) continue;
                CDataStream payload(peer.vSendMsg[i + 1], SER_NETWORK, PROTOCOL_VERSION);
                CBlockLocator locator;
                uint256 stop;
                payload >> locator >> stop;
                BOOST_REQUIRE(!locator.vHave.empty());
                BOOST_CHECK(locator.vHave.front() == (continued ? headers[2016].GetHash() : Params().GenesisBlock().GetHash()));
                BOOST_CHECK(stop == checkpoints.mapCheckpoints.rbegin()->second);
                requested = true;
            }
            BOOST_REQUIRE(requested);
            if (!replacement) {
                receive(peer, {headers.begin(), headers.begin() + 2000});
                receive(peer, {headers.begin() + 2000, headers.begin() + 2017});
                BOOST_CHECK(!peer.fDisconnect);
                BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
                if (scenario == 1) { SetMockTime(GetTime() + 61); send(peer); }
                else if (scenario == 2) { SetMockTime(start_time + 4 * 60 * 60 + 1); send(peer); }
                else peer.fDisconnect = true;
                BOOST_CHECK(peer.fDisconnect);
            } else if (scenario == 6) {
                receive(peer, {headers.begin() + 2017, headers.begin() + 2034});
                BOOST_CHECK(!peer.fDisconnect);
                // New transport and useful progress must NOT renew the original
                // four-hour lifetime. Finalization must not save expired state.
                SetMockTime(start_time + 4 * 60 * 60 + 1);
                send(peer);
                BOOST_CHECK(peer.fDisconnect);
                BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
            } else if (scenario == 5) {
                // Continuation reaches the checkpoint but still indexes nothing.
                receive(peer, {headers.begin() + 2017, headers.end()});
                BOOST_CHECK(!peer.fDisconnect);
                BOOST_CHECK(replay);
                BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
                // Replay must start at genesis and validate *all* commitments,
                // including those originally downloaded from peer A.
                receive(peer, {headers.begin(), headers.begin() + 2000});
                BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 2000);
                receive(peer, {headers.begin() + 2000, headers.begin() + 4000});
                receive(peer, {headers.begin() + 4000, headers.end()});
                BOOST_CHECK(!peer.fDisconnect);
                BOOST_CHECK_EQUAL(pindexBestHeader->nHeight, 4003);
                auto invalid = headers.back();
                invalid.hashPrevBlock = invalid.GetHash();
                ++invalid.nTime;
                MakeInvalidPoW(invalid);
                receive(peer, {invalid});
                BOOST_CHECK(!mapBlockIndex.count(invalid.GetHash()));
            } else {
                if (scenario == 0) {
                    // A replacement can extend the untrusted prefix but cannot
                    // substitute an alternative history for the hard checkpoint.
                    std::vector<CBlockHeader> fork(headers.begin() + 2017, headers.end());
                    ++fork.front().nNonce;
                    for (size_t i = 1; i < fork.size(); ++i) fork[i].hashPrevBlock = fork[i - 1].GetHash();
                    receive(peer, fork);
                } else if (scenario == 7) {
                    // An honest replacement may not know an untrusted locator
                    // and fall back to genesis. Reject rather than skip linkage.
                    receive(peer, {headers.begin(), headers.begin() + 2000});
                } else {
                    // An unavailable prefix/empty response discards the saved
                    // session; the next scenario must really start from zero.
                    receive(peer, {});
                }
                BOOST_CHECK(peer.fDisconnect);
                BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(checkpoint_authentication_retains_contextual_difficulty_checks)
{
    FastIBDOptions options;
    auto header = HeaderChain(1).front();
    header.nBits--;
    const CCheckpointData checkpoints{{{1, header.GetHash()}}};
    CheckpointTestParams params(checkpoints);
    Checkpoints::HeaderSync sync(0, Params().GenesisBlock().GetHash(), checkpoints);
    std::vector<CBlockHeader> out;
    BOOST_REQUIRE(sync.Process({header}, out));
    BOOST_REQUIRE(sync.Process({header}, out));
    CValidationState state;
    BOOST_CHECK(!ProcessNewBlockHeaders(out, state, params, nullptr, nullptr, &sync));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "bad-diffbits");
    BOOST_CHECK_EQUAL(mapBlockIndex.size(), 1U);
}

BOOST_AUTO_TEST_CASE(legacy_index_cannot_turn_claimed_work_into_verified_pow)
{
    FastIBDOptions options;
    auto header = HeaderChain(1).front();
    MakeInvalidPoW(header);
    const CCheckpointData checkpoints{{{1, header.GetHash()}}};
    CheckpointTestParams params(checkpoints);
    Checkpoints::HeaderSync sync(0, Params().GenesisBlock().GetHash(), checkpoints);
    std::vector<CBlockHeader> out;
    BOOST_REQUIRE(sync.Process({header}, out));
    BOOST_REQUIRE(sync.Process({header}, out));
    CValidationState state;
    BOOST_REQUIRE(ProcessNewBlockHeaders(out, state, params, nullptr, nullptr, &sync));
    FlushStateToDisk();
    // Simulate the old implementation's TREE entry, without either evidence
    // bit. Its claimed work must not be accepted on restart.
    {
        LOCK(cs_main);
        auto* index = mapBlockIndex.at(header.GetHash());
        index->nStatus &= ~(BLOCK_POW_CHECKED | BLOCK_CHECKPOINT_CHECKED);
        BOOST_REQUIRE(pblocktree->WriteBatchSync({}, 0, {index}));
        UnloadBlockIndex();
        BOOST_CHECK(!LoadBlockIndex(Params()));
    }
}

BOOST_AUTO_TEST_CASE(failed_parent_cannot_force_parallel_pow_work)
{
    FastIBDOptions options;
    const auto bad = BadBlock(Params().GenesisBlock().GetHash());
    ProcessNewBlock(Params(), bad, true, nullptr);
    BOOST_REQUIRE(mapBlockIndex.at(bad->GetHash())->nStatus & BLOCK_FAILED_MASK);
    auto headers = HeaderChain(2);
    headers[0].hashPrevBlock = bad->GetHash();
    headers[0].nTime = bad->nTime + 1;
    headers[1].hashPrevBlock = headers[0].GetHash();
    CValidationState state;
    BOOST_CHECK(!ProcessNewBlockHeaders(headers, state, Params()));
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "bad-prevblk");
    BOOST_CHECK(!headers[0].cache_init);
    BOOST_CHECK(!headers[1].cache_init);
}

BOOST_AUTO_TEST_SUITE_END()
