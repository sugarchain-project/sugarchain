v0.16.3.24-bloomy
-----------------
Final Release

**Changes:**
- Major Fix:
  * fix: Timeout downloading block
  * revert: download window back to 1024 (was 1024*120=122880)
  * add: checkpointData, chainTxData
  * add: seed list from dnsseed
  * fix: boost warnings on OSX by @volbil
  * fix: gitian github URL to official repo
- Minor Fix:
  * review: README.md by @Nugetzrul3
  * rename: Sugarchain Yumekawa developers
  * create: benchmark sh and result
  * add: translation: Korean, Japanese, Chinese(zh_CH)
  * add: bootstrap linearize at height 650000, 1043000
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


-------------


v0.16.3.21rc1
-------------
Mainnet Launching: `2019/08/24 15:00 UTC`
https://bitcointalk.org/index.php?topic=5177722.0

**Changes:**
- Block time: 5 seconds
- Difficulty: SugarShield-N510
- Block reward: 42.94967296 COIN `pow(2,32)/1E+08`
- Halving interval: 12500000 Blocks `pow(5,8)*32`
- Total supply: 1073741824 COIN `pow(2,30)`
- PoW algorithm: YespowerSugar `N2048 r32 (+pers)`
- Port: Main 34230/34229, Testnet 44230/44229, Regtest 45340/45339
- DEFAULT_MAX_TIME_ADJUSTMENT 70 (was 4200)
- MAX_FUTURE_BLOCK_TIME 60 (was 7200)
- QT delayed refresh balance [source](https://github.com/sugarchain-project/sugarchain/commit/72436c90b29844cf507895df053103f9b6840776#diff-2e3836af182cfb375329c3463ffd91f8)

**Credits:**  
Thanks to everyone who directly contributed to this release (alphabetical order)
- cryptozeny
- ilmango-doge
- okoto-xyz
- solardiz
- volbil
- zawy12
