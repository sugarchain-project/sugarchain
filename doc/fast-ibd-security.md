# Fast IBD: checkpoint authentication, not claimed work

Review baseline: PR [#225](https://github.com/sugarchain-project/sugarchain/pull/225),
`11a88a526e1d9a1b46e8755150e9d1eb58457a0e`, Sugarchain v0.16.3.
Performance objective: **complete network IBD within 12 hours on the target
machine**, with security taking priority over preserving the previous ~7 hours.
This is a design/implementation review, not a completed mainnet IBD measurement
or a proof that the historical checkpoint is correct. The PR description still
mentions the removed `-verify-ibd` option; this review follows the code revision,
not that stale description.

## Baseline findings

The baseline's network block entry point already calls real Yespower unless the
block is an ancestor of the last indexed hard-coded checkpoint. That prevents
the straightforward attack of sending an invented invalid-PoW block and getting
it connected through `ProcessNewBlock`.

However, `CheckProofOfWorkMeasured` returns true for **any** header while
`IsInitialBlockDownload()` is true, including beyond the latest checkpoint and
with checkpoints disabled. `CheckBlockHeader`, `CheckBlock`, and disk reads all
use this helper. Its warning incorrectly describes this as checkpoint-bound.
`AcceptBlockHeader` then calls `AddToBlockIndex`, which sets `BLOCK_VALID_TREE`,
adds nBits-derived work, updates `pindexBestHeader`, and dirties the index.

Consequences:

* A peer can append correctly linked, contextually valid but invalid-PoW headers
  after the real checkpoint and manufacture best-header work at SHA256d cost.
  Incorrect difficulty encodings are still rejected; obeying the adjustment
  formula is not evidence of mining.
* Before a checkpoint is reached, invented branches can allocate permanent
  index entries. A mismatching endpoint is rejected, but earlier entries are
  not rolled back. Repeated branches can consume memory/disk and derail header
  selection and block requests. A single peer can do this; majority is irrelevant.
* Best-header work is also an input to the existing assume-valid script shortcut.
  Invented work can distort that gate, though the assume-valid hash/ancestry
  tests remain. This is not evidence of a complete remote invalid-UTXO attack.
* `CheckBlock` and raw disk reads do not independently enforce PoW during IBD.
  The network block guard and indexed disk hash check still matter: it would be
  incorrect to claim that every such helper bypass already permits a remote
  attacker to activate arbitrary invalid-PoW blocks.
* Restarting trusts the index: `LoadBlockIndexGuts` does not recalculate Yespower.
  Switching the old binary to `-fast-ibd=0` does not audit previously skipped
  headers; duplicate headers return before PoW verification.

## Implemented model

Historical headers are authenticated **before** being admitted to the index.
Only one peer runs checkpoint presync while the latest checkpoint is missing.
No new checkpoints, peer voting, claimed-work anchors, or PoW samples were added.

1. Start from genesis or the last indexed existing checkpoint. In pass one,
   require consecutive SHA256d links, check every existing checkpoint at its
   exact height, and retain one full 256-bit endpoint hash per 2,000 headers.
   There is no CBlockIndex allocation, chainwork calculation, block request,
   best-header promotion, or database write for these untrusted headers.
   `getblockchaininfo.headers` therefore excludes presync data; separate presync
   progress is logged every 100,000 headers.
2. Only after matching the final hard-coded checkpoint are the stored endpoints
   authenticated. Restart header download from the start of the session.
3. Buffer at most one incomplete 2,000-header chunk during replay. Release a
   chunk only when its complete link chain matches its authenticated endpoint.
   Repacketizing the same chain is allowed; changing it is not. Trailing data
   beyond the final checkpoint is discarded and requested normally afterwards.
4. Replay goes through `AcceptBlockHeader` and `ContextualCheckBlockHeader`:
   difficulty adjustment, timestamp/MTP, version, checkpoint and failed-parent
   checks still run. Only the expensive historical Yespower calculation is
   replaced by checkpoint authentication.
5. All other headers require actual Yespower before index admission. Eligible
   batches precompute hashes with a bounded queue and up to eight workers,
   including the caller; acceptance remains ordered under `cs_main`. Workers
   mutate only per-header hash caches. At least the first new untrusted header
   must pass contextual/PoW checks before parallel batch work is scheduled.
6. Distinct persisted index bits record **actual PoW checked** and **checkpoint
   authenticated**. Neither TREE status, a timestamp, IBD state nor chainwork can
   set these bits. Hash lookup binds reuse to all 80 header bytes, including
   nBits. Target range checks still apply. Network objects cannot supply bits.
7. The common checker protects network blocks, compact block `CheckBlock`
   callers, and disk reads. Indexed reads also compare the loaded SHA256d hash
   with the expected index hash before proof reuse. Merkle/witness commitments,
   transaction and UTXO checks are not removed. The existing script assume-valid
   policy is unchanged.
8. At startup, legacy index entries without proof evidence are authenticated
   against an indexed checkpoint or actually PoW-verified before networking and
   activation. Invalid legacy PoW fails loading instead of becoming verified
   work. No existing chain files are deleted automatically.

Unsolicited historical HEADERS and INV-driven header requests from other peers
are deferred while the checkpoint session is needed; they cannot defeat the
quarantine by starting redundant bulk historical PoW verification.

Presync has a 60-second progress timeout and a four-hour absolute session
deadline. Authenticated replay retains the progress timeout, but is not capped
by the presync deadline. Neither timeout is a consensus rule; both phases enforce
their timeouts even for a whitelisted or sole peer. Finalizing a peer releases
the sync slot. INV-triggered
requests and ordinary chainwork-based peer eviction cannot issue competing
locators during presync. An unavailable or repeatedly malicious peer can still
waste bandwidth/time; there is no bounded completion guarantee under eclipse.

If the transport disconnects or hits the progress timeout during presync, one
in-memory, still-untrusted session can transfer to the next peer. Its exact last
header hash, sparse commitments, checkpoint configuration and original four-hour
deadline are retained. The replacement's GETHEADERS locator uses that hash and
genesis as fallback. No header is indexed and no validation evidence is created
by this transfer. All links and compiled checkpoints must still match, and the
entire authenticated history is replayed before index admission. Invalid or empty
responses discard the session; a peer that cannot extend a poisoned prefix may
be disconnected, after which a fresh session can start. Checkpoint changes and
expiration also discard it. Peer replacement cannot renew the absolute deadline.
Consequently a genuinely slow presync taking more than four hours still restarts;
the failover change addresses transport loss, not that separate resource limit.

`validation_block_tests/checkpoint_presync_peer_takeover` exercises real serialized
HEADERS messages, InitializeNode, SendMessages and FinalizeNode with a 4,003-header
chain. Before the fix, both disconnect and progress-timeout cases lost the first
2,017 headers: expecting a replacement at height 2,017 failed with height 0.
The regression checks the outbound locator, untrusted-state isolation, checkpoint
and deadline invalidation, malicious continuation, and successful authentication
plus replay and post-checkpoint PoW rejection. No mainnet download is needed.

After endpoint authentication, one immutable commitment snapshot survives peer
loss or rejection. A replacement peer resumes replay at the highest exact
commitment hash already indexed at its expected height, valid at TREE level and
marked checkpoint-authenticated. It must still match every remaining commitment
and pass contextual header validation. An unaccepted or partial chunk is fetched
again; a failed peer cannot alter the snapshot. Checkpoint configuration must
match before reuse. Completion clears the snapshot. It is memory-only: a process
restart still starts presync at the last indexed compiled-in checkpoint.

The September 25 replay interruption was the original four-hour cap, not an
invalid header at 17.59M: the session started at September 24 23:19:40 and logged
the checkpoint timeout at 03:19:41, immediately after accepting height 17,590,000.
The replacement started at indexed checkpoint 6,513,497, giving 2,000-header
responses and 100,000-height progress logs their observed +1497 remainder.

At 37.5 million headers, endpoints contain 600,000 bytes of hashes (vector
capacity can be about 1 MiB), with one additional immutable snapshot of those
hashes, plus a bounded chunk and a bounded set of released
hashes. Packet boundary overlap can release two chunks in one call; it cannot
make storage proportional to the number of forks. Header traffic increases by
roughly 37.5M * 81 bytes = 3.04 GB for a genesis-start first pass, plus framing.
The permanent index contains only authenticated or actually verified headers.

The download-twice architecture has a precedent in Bitcoin Core's
[headers presync](https://github.com/bitcoin/bitcoin/blob/master/src/headerssync.h),
but its trust model is different: Bitcoin verifies PoW before using a work
threshold and stores probabilistic commitments. This implementation uses full
SHA256d commitments authenticated to an existing hard-coded endpoint. It is not
a backport of Bitcoin's mechanism and does not inherit a claim of its security.

## Answers to the security questions

1. **Assumption:** the checkpoint's selected history has valid historical PoW;
   the software/checkpoint source and local validation database are trusted;
   SHA256d has the required collision/second-preimage resistance. A fixed block
   hash commits to its nBits, previous hash and Merkle root, recursively to its
   ancestors. It does not itself prove those ancestors satisfy Yespower.
2. **Joining an invented history to that hash:** under these assumptions, the
   attacker cannot replace an ancestor while keeping the checkpoint hash. At
   the divergence/reconnection boundary they need to break hash binding.
   Merely knowing or replaying the real checkpoint header is insufficient.
3. **Pre-checkpoint DoS:** possible in the baseline, with persistent allocation.
   Presync removes free unverified-index growth, but not traffic, hashing, slow
   service, repeated connection attempts or eclipse. Existing early checkpoints
   detect bad prefixes sooner; the large 6,513,497-to-37,500,000 gap still permits
   a long failed first pass. Timeouts bound each attempt, not all attempts.
4. **Minimum work alone:** unsafe if PoW is unchecked. It is arithmetic over
   target claims, not evidence that the corresponding work was done.
5. **Checkpoint plus minimum work:** authentication establishes the history;
   the work threshold then filters insufficient authenticated/verified chains
   and feeds IBD/download/assume-valid gates. It adds no independent validity
   proof to the checkpoint and does not guarantee a fresh tip.
6. **Multiple checkpoints:** useful for early mismatch rejection and restarting
   from already indexed anchors. They do not strengthen the cryptographic
   binding of an already authenticated final endpoint or independently prove
   historical PoW. The existing list is unchanged.
7. **After checkpoint:** actual Yespower is mandatory. It need not be computed
   twice for identical header bytes: an actual checked result can be reused
   for the network block and disk read. All block body checks still apply.
8. **Invalid chainstate:** a malicious peer cannot substitute a different
   checkpoint-bound history without breaking hash binding. Nevertheless a
   wrong/malicious checkpoint can commit invalid historical PoW; fast mode
   cannot independently detect that. Other validity checks remain, subject to
   the existing assume-valid script assumption and ordinary implementation bugs.
9. **One or many malicious peers:** can withhold, stall, send failed presyncs,
   replay traffic, attempt CPU DoS with verification batches, or eclipse the
   node. They cannot vote an arbitrary chain into truth. Valid-PoW forks remain
   subject to ordinary work selection and checkpoint restrictions. Endpoint
   replay equivocation cannot authorize different headers.
10. **Full fallback:** `-fast-ibd=0` disables checkpoint proof substitution and
    audits checkpoint-only legacy/persisted evidence during startup. Already
    actually checked identical headers may reuse their verified result; import
    and reindex do actual PoW. Full mode can exceed 12 hours. For an independent
    *script* audit as well, use a fresh validation/reindex with `-assumevalid=0`;
    changing a startup option does not undo existing cached script state.

Persistent proof bits are validation caches, not attestations against a local
attacker. A compromised database or malicious binary can forge them, just as it
can forge existing script-validation flags or UTXO data. Changing checkpoint
policy across releases is a trust-policy change; use full mode/reindex when
withdrawing trust in an earlier anchor. A crash during replay can leave a valid
checkpoint-authenticated prefix; its evidence survives restart without treating
an unauthenticated first pass as durable state.

## Performance budget and measurement limits

The actual header PoW hash is Yespower, while links and checkpoint identifiers
use SHA256d. Benchmarked the repository's compiled Yespower library on the
available Ryzen 9 5950X (16 physical cores / 32 logical CPUs), 15 seconds per
worker count, with other applications running:

| Workers | Raw hashes/s | 44.5M raw hash time |
| --- | ---: | ---: |
| 1 | 345.4 | 35.79 h |
| 4 | 1,293.4 | 9.56 h |
| 8 | 2,219.0 | 5.57 h |
| 16 | 1,407.4 | 8.78 h |

Reproduce with `contrib/bench/yespower-throughput.cpp`. These are short raw hash
measurements, not sustained header processing, a controlled hardware comparison,
or an end-to-end IBD result. In particular, 16 workers were slower; linear
core-count extrapolation would be wrong. Eight total workers are the default
upper bound, not a universal optimum on other CPUs.

Verifying all 44.5M headers serially violates the objective. Parallel verification
of every header remains uncertain within 12 hours: even the best measured raw
cost adds ~5.6 hours, before contextual validation, network and block processing.
Sampling does not provide full validation, and fixed samples can be evaded on an
unanchored fork. Spending the budget on authentication before admission and
actual post-checkpoint verification has a clearer security benefit.

With tip height 44.5M, the implemented path actually hashes the ~7M post-checkpoint
headers once: ~0.88 hours at the eight-worker raw rate, or ~5.8–7.8 hours at the
user's serial 250–333/s rate. Recomputing that suffix separately for blocks and
disk reads would waste another large part of the budget; the evidence cache
removes that repeated calculation without accepting an unchecked suffix.

A **planning budget, not a measurement**, on the target machine is:

| Stage | Budget |
| --- | ---: |
| Extra first-pass header download/authentication | 1.0 h |
| Checkpoint replay and historical contextual checks | 2.0 h |
| Actual post-checkpoint header PoW and context/network | 1.5 h |
| Block bodies, scripts/UTXO, writes and activation with proof reuse | 6.0 h |
| Startup/shutdown/flush and margin | 1.5 h |
| Total ceiling | 12.0 h |

This requires roughly >=10,417 first-pass headers/s, >=5,208 replay headers/s,
>=1,297 suffix headers/s, and >=2,061 blocks/s for those respective budgets.
These rates are plausible from the supplied measurements and the raw benchmark,
but **a fresh full mainnet run has not established them for this implementation**.
On slower hardware, slow/high-latency peers, growing chain height, storage pressure
or a different script policy, the objective may fail. Do not describe this build
as a demonstrated <=12-hour IBD release without the following measurement.

Record a cold run's commit/config, CPU/thread count/load, memory/swap, disk,
network and peers, start/end heights and total wall time through durable final
flush and IBD exit. Separately record presync, replay, suffix and block times;
actual Yespower call counts/cache reuse; HS/BS MA60; peak untrusted memory,
permanent index entries, dirty-entry memory, bytes/retries/disconnections;
script/UTXO/flush costs and final tip/chainwork. Test interruption/restart during
both passes and during block download, peer failure, checkpoint-disabled mode,
full mode, and upgrade from an old unverified index. Benchmark on the intended
host without the concurrent builds/tests used during development.

## Validation

Security tests cover unproven header/block/disk rejection; no first-pass index
promotion; every-checkpoint matching; altered replay chunks; changed packet
boundaries; proof/chain-parameter mismatch; checkpoint boundary; full/checkpoint-
disabled/import/reindex modes; Merkle and contextual difficulty checks; ordered
parallel PoW failures; exact-header cache reuse; persistent evidence encoding;
legacy index rejection; and actual P2P header authentication, unsolicited-peer deferral, malicious peer
failover and progress timeout. Failed parents are rejected before parallel work
can be scheduled. See `src/test/validation_block_tests.cpp`.

The separate `DoS_tests` ordinary slow-chain eviction test explicitly uses full
mode so it tests that policy rather than checkpoint-presync deadlines. An explicit
Boost bind includes fix pre-existing missing-include build errors exposed by
recompiling validationinterface.cpp and the Qt signal users with the installed
Boost version.

`make check` additionally runs the IBD-specific rejection tests in fresh
processes: the process-global IBD latch may already be false by the time the full
unit suite reaches these cases. Final command results are recorded below. No existing
mainnet data directory was modified or reindexed for these tests.


Results on the review host (2026-09-24):

| Check | Result |
| --- | --- |
| Daemon, Qt, core test and Qt test builds | PASS (explicit Boost includes required) |
| `test_sugarchain --run_test=validation_block_tests` | PASS, 12 cases |
| `test_sugarchain --run_test=pow_tests,DoS_tests` | PASS, 11 cases |
| `test_sugarchain --log_level=test_suite` | PASS, all 293 cases; final run ~66 s |
| Fresh-process invalid-PoW header/block/disk and IBD block tests | PASS, both cases, no IBD-latch skip |
| Checkpoint-mode `fChecked` object rechecked in full mode | PASS |
| Isolated daemon: mine 30 regtest blocks, restart with fast mode disabled, `verifychain 4 0` | PASS; same tip and 30 headers/blocks in both modes |
| CLI utility, secp256k1 (2), UniValue (3) tests | PASS |
| Extended `make -C src -j4 check` | FAIL only in Qt PaymentServerTests: unchanged fixture certificate expired 2022-12-08; merchant expected `testmerchant.org`, actual empty |
| `git diff --check` | PASS |
| Complete new mainnet IBD <=12 h | NOT MEASURED |

The Qt certificate validation was not disabled or relaxed to make that test
pass. The IBD changes are a checkpoint-trust design with tested invariants and a
plausible target-host performance budget, **not an independently audited release
or a demonstrated 12-hour mainnet run**. Full IBD timing remains the release
acceptance measurement. Detailed command output for this session is in
`/tmp/sugarchain-pr225-review/` on the review host.
