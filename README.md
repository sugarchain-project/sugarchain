Sugarchain Yumekawa
===================

[![Build Status](https://travis-ci.com/cryptozeny/sugarchain-v0.16.3.svg?token=KrYycpAcc7g95pAVRykP&branch=master-v0.16.3)](https://travis-ci.com/cryptozeny/sugarchain-v0.16.3)

What is Sugarchain?
-------------------

Sugarchain is a PoW blockchain for ultra speed transaction (5 seconds block time) and fair distribution (CPU mining only by Yespower 1.0.1).
 - `one-CPU-one-vote` by Satoshi Nakamoto
 - `NVIDIA, fuck you` by Linus Torvalds
 - the world's fastest PoW blockchain (2019-06-26)

The meaning of Yumekawa
-----------------------

Sugarchain's first node software is called `Yemekawa (夢川)`. It can be translated in some ways.
 - "Yume (夢)" means dream and "Kawa (川)" means river. So its `Dream River` in japanese.
 - The second letter "Kawa" stands for "Kawaii (可愛い)". In this case the meaning is `Dreamy Cute`.
 - Also Yumekawa is replaces the word `Core` (ie: Bitcoin Core). We think it sounds a bit centralized.

License
-------

Sugarchain Yumekawa is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.
- Copyright (c) 2009-2010 Satoshi Nakamoto
- Copyright (c) 2009-2018 The Bitcoin Core developers
- Copyright (c) 2013-2019 Alexander Peslyak - Yespower 1.0.1
- Copyright (c) 2016-2018 The Zcash developers - DigiShieldZEC
- Copyright (c) 2018-2019 The Sugarchain developers

Depends on Bitcoin Core
--------------

Exactly the same as dependencies of `Bitcoin Core v0.16.3`.

```bash
sudo add-apt-repository -y ppa:bitcoin/bitcoin && \
sudo apt-get update && \
sudo apt-get install -y \
software-properties-common libdb4.8-dev libdb4.8++-dev \
build-essential libtool autotools-dev automake pkg-config \
libssl-dev libevent-dev bsdmainutils libboost-all-dev \
libminiupnpc-dev libzmq3-dev libqt5gui5 libqt5core5a \
libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev \
protobuf-compiler libqrencode-dev help2man
```

Build
-----

 * Make

```bash
git clone git@github.com:cryptozeny/sugarchain-v0.16.3.git && \
cd sugarchain-v0.16.3 && \
./autogen.sh && \
./configure && \
make -j$(nproc) && \
make check -j$(nproc)
```

 * (optional) Reduce binary size using strip (about 90% file size reduction)

```bash
strip ./src/sugarchain-cli && \
strip ./src/sugarchaind && \
strip ./src/qt/sugarchain-qt && \
strip ./src/sugarchain-tx && \
strip ./src/test/test_sugarchain
```

 * (optional) After make, update binary docs (manpages) using help2man `.1` files

```bash
make -j$(nproc) && ./contrib/devtools/gen-manpages.sh
```

 * (optional) build for Windows and OSX you may need `--disable-shared` option with make.

Unit Test
---------

Every Yumekawa developers should check this unit test. Some updates may break this tests sometimes.

 * Test All
 ```bash
 ./src/test/test_sugarchain test_bitcoin --log_level=test_suite
 ```
 
 * (optional) Test Partially: ie `blockencodings_tests`
 ```bash
 ./src/test/test_sugarchain test_bitcoin --log_level=test_suite --run_test=blockencodings_tests
 ```
 
 * (optional) Test QT (GUI)
 ```bash
 ./src/qt/test/test_sugarchain-qt
 ```

Run
---

The options `-rpcuser` and `-rpcpassword` can be freely choose. If you need the file `debug.log`, then drop the `-printtoconsole` out. `server=1` needed by RPC servers or cpuminer (when solo-mining).

 * Testnet [debug mode]
 > ./src/qt/sugarchain-qt -testnet -server=1 -rpcuser=rpcuser -rpcpassword=rpcpassword -printtoconsole

 * Testnet [debug mode for PoW]
 > ./src/qt/sugarchain-qt -testnet -server=1 -rpcuser=rpcuser -rpcpassword=rpcpassword -debug=pow -printtoconsole

 * Reference https://en.bitcoin.it/wiki/Running_Bitcoin

CLI
---

 * Testnet
 > ./src/sugarchain-cli -testnet -rpcuser=rpcuser -rpcpassword=rpcpassword getblockcount

Release process using GITIAN
----------------------------

https://gist.github.com/cryptozeny/3501c77750541208b9dd1a9e9719fc53

 * Every Sugarchain Yumekawa developers should do following GITIAN release process. It's the safest way to distribute binaries to people.
 * Please use GITIAN release with checking PGP signature, or compile it yourself on your own machine.
 