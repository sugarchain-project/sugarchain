Yumekawa v0.16.3.24-bloomy
--------------------------
Final Release

**Changes:**
- Major Fix:
  * fix: Timeout downloading block
  * revert: download window back to `1024` (was 1024*120=122880)
  * add: checkpointData, chainTxData
  * add: seed list from dnsseed
  * fix: boost warnings on OSX by @volbil
  * fix: gitian github URL to official repo
- Minor Fix:
  * review: README.md by @Nugetzrul3
  * rename: Sugarchain Yumekawa developers
  * create: benchmark sh and result
  * add: translation: Korean, Japanese, Chinese(zh_CH)
  * add: bootstrap linearize at height `650000`, `1043000`
  * fix: comment error (#6)

**Known Issues:**  
https://github.com/sugarchain-project/sugarchain#known-issues

**Credits:**  
Thanks to everyone who directly contributed to this release (alphabetical order)

- AestheticSenpai
- cryptozeny
- ilmango-doge
- joeland1
- nao20010128nao
- Nugetzrul3
- okoto-xyz
- RicK~Z
- solardiz
- volbil
- y-chan
- zawy12


----------------------


Yumekawa v0.16.3.21rc1
----------------------
Mainnet Launching: `2019/08/24 15:00 UTC`  
https://bitcointalk.org/index.php?topic=5177722.0

**Changes:**
- Block time: `5` seconds
- Difficulty: [SugarShield-N510](https://github.com/sugarchain-project/sugarchain/blob/master-v0.16.3/src/pow.cpp)
- Block reward: 42.94967296 COIN `pow(2,32)/1E+08`
- Halving interval: 12500000 Blocks `pow(5,8)*32`
- Total supply: 1073741824 COIN `pow(2,30)`
- PoW algorithm: YespowerSugar
  * Settings
  ```cpp
  yespower_params_t yespower_1_0_sugarchain = {
    .version = YESPOWER_1_0,
    .N = 2048,
    .r = 32,
    .pers = (const uint8_t *)"Satoshi Nakamoto 31/Oct/2008 Proof-of-work is essentially one-CPU-one-vote",
    .perslen = 74
  };
  ```
- Genesis:
  * Timestamp [link](https://www.thetimes.co.uk/article/facebook-s-libra-knocks-bitcoin-b3zvn67k0)
  ```cpp
  "The Times 17/July/2019 Bitcoin falls after senators call Facebook delusional over libra"
  ```
  * Genesis block
  ```cpp
  genesis = CreateGenesisBlock(1565881200, 247, 0x1f3fffff, 1, 42.94967296 * COIN);
  ```
- Port: Main `34230/34229`, Testnet 44230/44229, Regtest 45340/45339
  * Meaning: 34230 is the molar mass of a sugar [link](https://twitter.com/cryptozeny/status/1130167161475911682)
- DEFAULT_MAX_TIME_ADJUSTMENT `70` (was 4200)
- MAX_FUTURE_BLOCK_TIME `60` (was 7200)
- QT delayed refresh balance [source](https://github.com/sugarchain-project/sugarchain/commit/72436c90b29844cf507895df053103f9b6840776#diff-2e3836af182cfb375329c3463ffd91f8)

**Credits:**  
Thanks to everyone who directly contributed to this release (alphabetical order)
- cryptozeny
- ilmango-doge
- okoto-xyz
- solardiz
- volbil
- zawy12
