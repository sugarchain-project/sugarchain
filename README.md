Sugarchain Yumekawa
===================

[![Build Status](https://travis-ci.com/cryptozeny/sugarchain-v0.16.3.svg?token=KrYycpAcc7g95pAVRykP&branch=master-v0.16.3)](https://travis-ci.com/cryptozeny/sugarchain-v0.16.3)

 - Copyright (c) 2009-2010 Satoshi Nakamoto
 - Copyright (c) 2009-2018 The Bitcoin Core developers
 - Copyright (c) 2013-2018 Alexander Peslyak - Yespower 1.0.0
 - Copyright (c) 2016-2018 The Zcash developers - DigiShieldZEC
 - Copyright (c) 2018-2019 The Sugarchain developers

-----

## The meaning of Yumekawa
Sugarchain's first node software is called `Yemekawa (夢川)`. It can be translated in some ways.
 - "Yume (夢)" means dream and "Kawa (川)" means river. So its `Dream River` in japanese.
 - The second letter "Kawa" stands for "Kawaii (可愛い)". In this case the meaning is `Dreamy Cute`.
 - Also Yumekawa is replaces the word `Core` (ie: Bitcoin Core). It sounds a bit centralized and we don't want such [Elitism](https://en.wikipedia.org/wiki/Elitism). In this term we are Yumekawa developers, but not ~~Core developers~~.

-----

## Depends on BTC
It is exactly same as the dependencies of `BTC v0.16.3`. It is not necessary if you already have.

```bash
sudo add-apt-repository ppa:bitcoin/bitcoin && \
sudo apt-get update && \
sudo apt-get install \
software-properties-common libdb4.8-dev libdb4.8++-dev \
build-essential libtool autotools-dev automake pkg-config \
libssl-dev libevent-dev bsdmainutils libboost-all-dev \
libminiupnpc-dev libzmq3-dev libqt5gui5 libqt5core5a \
libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev \
protobuf-compiler libqrencode-dev
```

## Build
```bash
git clone git@github.com:cryptozeny/sugarchain-v0.16.3.git && \
cd sugarchain-v0.16.3 && \
./autogen.sh && \
./configure && \
make -j$(nproc) && \
make check -j$(nproc)
```

## Unit Test
Every Yumekawa developers should check this unit test. Some updates may break this tests sometimes.

 * Test All
 ```bash
 ./src/test/test_sugarchain test_bitcoin --log_level=test_suite
 ```
 
 * Test Partially: ie `blockencodings_tests`
 ```bash
 ./src/test/test_sugarchain test_bitcoin --log_level=test_suite --run_test=blockencodings_tests
 ```
 
 * Test QT (GUI)
 ```bash
 ./src/qt/test/test_sugarchain-qt
 ```

## Run
The options `-rpcuser` and `-rpcpassword` can be freely choose. If you need the file `debug.log`, then drop the `-printtoconsole` out. `server=1` needed by RPC servers or cpuminer(solo-mining).

 * Testnet [debug mode]
 > ./src/qt/sugarchain-qt -testnet -server=1 -rpcuser=rpcuser -rpcpassword=rpcpassword -addnode=explorer-testnet.cryptozeny.com -printtoconsole

 * Testnet [debug mode for PoW]
 > ./src/qt/sugarchain-qt -testnet -server=1 -rpcuser=rpcuser -rpcpassword=rpcpassword -addnode=explorer-testnet.cryptozeny.com -debug=pow -printtoconsole

## CLI
 * Testnet
 > ./src/sugarchain-cli -testnet -rpcuser=rpcuser -rpcpassword=rpcpassword getblockcount

## Release process using GITIAN

https://gist.github.com/cryptozeny/3501c77750541208b9dd1a9e9719fc53

 * Every Sugarchain Yumekawa developers should do following this GITIAN release process. This is the safest method to release.
 * Do not trust created binaries by any others, and please use released by GITIAN and verify the PGP signatures of the Sugarchain Yumekawa developers.
