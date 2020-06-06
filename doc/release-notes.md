## Yumekawa v0.16.3.34-starboy
- Version: Final Release
- Date: 2020-06-07

**Changes:**
- Major Fix:
  * Fix: IBD 30% faster, and 60% reduced data traffic
    [#122](https://github.com/sugarchain-project/sugarchain/pull/122)
  * Add: new option -prunedebuglogfile: limit filesize of debug.log
    [#110](https://github.com/sugarchain-project/sugarchain/pull/110)
  * Add: when IBD, print blockheader count on debug.log (only sugarchain-qt)
    [#128](https://github.com/sugarchain-project/sugarchain/pull/128)
  * Update: checkpoints (mainnet)
    [#132](https://github.com/sugarchain-project/sugarchain/pull/132)
  * Fix: IBD optimizing
    [#135](https://github.com/sugarchain-project/sugarchain/pull/135)
    [#22 (comment)](https://github.com/sugarchain-project/sugarchain/pull/22#issuecomment-568301895)
  * Update: seed list
    [#144](https://github.com/sugarchain-project/sugarchain/pull/144)
  * Remove: BCLog::POW (-debug=pow)
    [#142](https://github.com/sugarchain-project/sugarchain/pull/142)
- Minor Fix:
  * GUI: display size in MB (was GB)
    [#125](https://github.com/sugarchain-project/sugarchain/pull/125)
  * Revert: IBD settings back to BTC original
    [#124](https://github.com/sugarchain-project/sugarchain/pull/124)
  * Add: bootstrap height at 4421701
    [#143](https://github.com/sugarchain-project/sugarchain/pull/143)
  * Fix: travis pathlib2
    [#148](https://github.com/sugarchain-project/sugarchain/pull/148)

**Known Issues**
- Transaction too large:
  * This is a part of BTC.
  * It will be fixed in next *Taproot+Schnorr* update.
- Slow update balance on wallet:
  * This slow is a part of BTC.
  * Update total balance *every minute (12 blocks)* interval.
  * This fix is a (nice) workaround for now. [source](https://github.com/sugarchain-project/sugarchain/commit/72436c90b29844cf507895df053103f9b6840776#diff-2e3836af182cfb375329c3463ffd91f8)
- Poor performance on ARM CPUs (32/64-Bit):
  * No ARM optimization for Yespower yet.
- Poor performance on 32-Bit OS:
  * No SSE2 optimization for Yespower yet. [source](https://github.com/sugarchain-project/sugarchain/blob/d977987a83aba115d50a9130f0d7914330d1bc75/src/crypto/yespower-1.0.1/yespower-opt.c#L59)
- Slow startup on low memory machines, due to blockchain growth.
  * Startup can take up to 5 hours on 1cpu 1024ram (+swap 3GB) VPS.
  * Workaround is just increase RAM at least 2 GB.

**Credits:**
Thanks to everyone who directly contributed to this release
- decryp2kanon
- solardiz (original Yespower author)
- barrystyle
- volbil
- Nugetzrul3
- cryptozeny

**Financial Support:**
We give our utmost thanks to the donors. This donation will be used for our future development. See [donations](https://github.com/sugarchain-project/Donations/blob/master/README.md).
- Rakutens

-----

## Yumekawa v0.16.3.30-moonlight
- Version: Final Release
- Date: 2020-03-02

**Changes:**
- Major Fix:
  * add: Yespower cached PoW
    [#22](https://github.com/sugarchain-project/sugarchain/pull/22)
  * fix: enhanced Yespower cached PoW
    [#61](https://github.com/sugarchain-project/sugarchain/pull/61)
    [#69](https://github.com/sugarchain-project/sugarchain/pull/69)
  * review: solardiz (original Yespower author) confirmed
    [1](https://github.com/sugarchain-project/sugarchain/pull/22#issuecomment-568301895)
    [2](https://github.com/sugarchain-project/sugarchain/pull/22#issuecomment-568306963)
    [3](https://github.com/sugarchain-project/sugarchain/pull/22#issuecomment-568505605)
    [4](https://github.com/sugarchain-project/sugarchain/issues/31#issuecomment-583833642)
    [5](https://github.com/sugarchain-project/sugarchain/issues/32#issuecomment-568302174)
  * fix: IBD: do not disconnect *whitelisted* peers during IBD
    [#80](https://github.com/sugarchain-project/sugarchain/pull/80)
  * remove: IBD: disable additional download during IBD, due to too much traffic
    [#78](https://github.com/sugarchain-project/sugarchain/pull/78)
  * add: chainwork, assumevalid, txdata, checkpoint and seeds
    [#50](https://github.com/sugarchain-project/sugarchain/pull/50)
    [#52](https://github.com/sugarchain-project/sugarchain/pull/52)
- Minor Fix:
  * fix: IBD: do not print this connection log during IBD
    [#78](https://github.com/sugarchain-project/sugarchain/pull/78)
  * add: bootstrap block height at 2601001
    [#49](https://github.com/sugarchain-project/sugarchain/pull/49)
  * add: kanon PGP
    [#48](https://github.com/sugarchain-project/sugarchain/pull/48)
  * fix: copyright
    [#75](https://github.com/sugarchain-project/sugarchain/pull/75)
    [#90](https://github.com/sugarchain-project/sugarchain/pull/90)
    [#96](https://github.com/sugarchain-project/sugarchain/pull/96)
  * fix: makeseeds.py version
    [#89](https://github.com/sugarchain-project/sugarchain/pull/89)
  * cleanup:
    [#60](https://github.com/sugarchain-project/sugarchain/pull/60)
    [#66](https://github.com/sugarchain-project/sugarchain/pull/66)
    [#84](https://github.com/sugarchain-project/sugarchain/pull/84)
    [#85](https://github.com/sugarchain-project/sugarchain/pull/85)
    [#92](https://github.com/sugarchain-project/sugarchain/pull/92)

**Known Issues**
Same as before

**Credits:**
Thanks to everyone who directly contributed to this release
- solardiz (original Yespower author)
- volbil
- okoto-xyz
- decryp2kanon
- cryptozeny

**Financial Support:**
We give our utmost thanks to the donors. This donation will be used for our future development. See [donations](https://github.com/sugarchain-project/Donations/blob/master/README.md).

- 唐伯虎
- Rakutens
- [happysensor.xyz](http://happysensor.xyz/)

-----

## Yumekawa v0.16.3.24-bloomy
- Version: Final Release
- Date: 2019-11-10

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

**Known Issues**
- Transaction too large:
  * This is a part of BTC.
  * It will be fixed in next *Schnorr Signature* update.
- Slow update balance on wallet:
  * Update total balance *every minute (12 blocks)* interval.
  * This slow is a part of BTC.
  * This fix is a (nice) workaround for now. [source](https://github.com/sugarchain-project/sugarchain/commit/72436c90b29844cf507895df053103f9b6840776#diff-2e3836af182cfb375329c3463ffd91f8)
- Poor performance on ARM CPUs (32/64-Bit):
  * ARM optimization for Yespower disabled for now.
- Poor performance on 32-Bit OS:
  * SSE2 for Yespower disabled for now. [source](https://github.com/sugarchain-project/sugarchain/blob/d977987a83aba115d50a9130f0d7914330d1bc75/src/crypto/yespower-1.0.1/yespower-opt.c#L59)
  * Please use *64-bit* for best performance.

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

-----

## Yumekawa v0.16.3.21rc1
- Version: The First Release
- Date: 2019-08-24

Mainnet Launching: `2019/08/24 15:00 UTC`
https://bitcointalk.org/index.php?topic=5177722.0

**Changes:**
- Block time: `5` seconds
- Difficulty: [SugarShield-N510](https://github.com/sugarchain-project/sugarchain/blob/master-v0.16.3/src/pow.cpp)
- Block reward: 42.94967296 COIN `pow(2,32)/1E+08`
- Halving interval: 12500000 Blocks `pow(5,8)*32` [source](https://github.com/sugarchain-project/sugarchain/blob/32c7d945cda04374f1abbcb8e9787704e7171d4e/src/validation.cpp#L1168-L1211)
- Total supply: 1073741824 COIN `pow(2,30)` [source](https://github.com/sugarchain-project/yumekawa-utils)
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
