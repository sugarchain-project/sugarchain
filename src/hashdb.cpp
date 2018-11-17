// Copyright (c) 2014-2018 The BitZeny developers
// Copyright (c) 2018 The Sugarchain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hashdb.h>

std::unique_ptr<CHashDB> phashdb;

CHashDB::CHashDB(size_t nCacheSize, bool fMemory, bool fWipe) : CDBWrapper(GetDataDir() / "hashes", nCacheSize, fMemory, fWipe) {}

uint256 CHashDB::GetHash(const CBlockHeader &block) {
    uint256 hash;
    if (!this->Read(block, hash)) {
        hash = block.GetHash();
        this->Write(block, hash);
    }
    return hash;
}
