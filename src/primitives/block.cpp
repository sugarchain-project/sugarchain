// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <utilstrencodings.h>
#include <crypto/common.h>

// yespower
#include <stdlib.h>
#include "crypto/yespower-1.0.0/yespower.h"
#include "streams.h"
#include "version.h"

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

// yespower
uint256 CBlockHeader::GetPoWHash() const
{
    uint256 thash;
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *this;
    yespower_params_t yespower_1_0_sugarchain = {
        .version = YESPOWER_1_0,
        .N = 2048,
        .r = 32,
        .pers = (const uint8_t *)"Sugarchain: Decentralized Cryptocurrency for one-CPU-one-vote",
        .perslen = 61
    };
    if (yespower_tls( (unsigned char *)&ss[0], ss.size(), &yespower_1_0_sugarchain, (yespower_binary_t *)&thash) ) {
        abort();
    }
    return thash;
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
