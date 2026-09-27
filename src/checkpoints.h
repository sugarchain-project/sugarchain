// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHECKPOINTS_H
#define BITCOIN_CHECKPOINTS_H

#include <chainparams.h>
#include <primitives/block.h>
#include <uint256.h>

#include <set>
#include <vector>

#include <map>
#include <memory>

class CBlockIndex;
struct CCheckpointData;

/**
 * Block-chain checkpoints are compiled-in sanity checks.
 * They are updated every release or three.
 */
namespace Checkpoints
{

/** Two-pass authentication of historical headers. The first pass records only
 * sparse SHA256d commitments, never chainwork or block-index entries. Reaching
 * the hard-coded endpoint authenticates those commitments. The second pass
 * releases a chunk only after its complete hash chain matches a commitment.
 * Presync download state may move between peers while remaining untrusted.
 * An authenticated snapshot may survive the peer to resume replay at an
 * indexed, authenticated chunk boundary.
 */
class HeaderSync {
public:
    static constexpr size_t CHUNK_SIZE = 2000;
    HeaderSync(int start_height, const uint256& start_hash, const CCheckpointData& checkpoints);
    bool Process(const std::vector<CBlockHeader>& headers, std::vector<CBlockHeader>& authenticated);
    bool Authenticates(const uint256& hash, const CCheckpointData& checkpoints) const;
    /** Clone authenticated commitments, discarding all packet-local state.
     * Requires cs_main; resumes only at an accepted commitment boundary. */
    std::shared_ptr<HeaderSync> Resume(const CCheckpointData& checkpoints) const;
    /** Transport progress only: never authorizes indexing or PoW reuse. */
    bool CanContinuePresync(const CCheckpointData& checkpoints) const {
        return !m_failed && !m_replaying && m_height > m_start_height &&
               m_checkpoints == checkpoints.mapCheckpoints;
    }
    uint256 NextHash() const { return m_prev; }
    uint256 StopHash() const { return m_checkpoints.rbegin()->second; }
    bool Complete() const { return m_complete; }
    bool Replaying() const { return m_replaying; }
    int Height() const { return m_height; }
    int StartHeight() const { return m_start_height; }
    int StopHeight() const { return m_checkpoints.rbegin()->first; }
    size_t CommitmentCount() const { return m_commitments.size(); }
private:
    const int m_start_height;
    const uint256 m_start_hash;
    const MapCheckpoints m_checkpoints;
    int m_height;
    uint256 m_prev;
    bool m_replaying{false};
    bool m_complete{false};
    bool m_failed{false};
    size_t m_replay_chunk{0};
    std::vector<uint256> m_commitments;
    std::vector<CBlockHeader> m_buffer;
    std::set<uint256> m_authenticated;
};

//! Returns false when a block at a checkpoint height has the wrong hash.
bool CheckBlock(int nHeight, const uint256& hash, const CCheckpointData& data);

//! Returns last CBlockIndex* in mapBlockIndex that is a checkpoint
CBlockIndex* GetLastCheckpoint(const CCheckpointData& data);

//! Returns whether pindex is on the history committed to by the last known checkpoint.
bool IsAncestorOfLastCheckpoint(const CBlockIndex* pindex, const CCheckpointData& data);

} //namespace Checkpoints

#endif // BITCOIN_CHECKPOINTS_H
