# RabidCoin Build Instructions

## Requirements (Ubuntu 22.04/24.04)
```
sudo apt install build-essential libtool autotools-dev automake pkg-config \
  libssl-dev libevent-dev bsdmainutils libboost-all-dev \
  libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools \
  libprotobuf-dev protobuf-compiler libqrencode-dev \
  g++-mingw-w64-x86-64 mingw-w64
```

## Build Linux Wallet
```
./autogen.sh
./configure --with-gui=qt5 --disable-tests --disable-bench
make -j$(nproc) src/qt/rabidcoin-qt
strip src/qt/rabidcoin-qt
```

## Build Windows Wallet (cross-compile from Linux)
```
# Build depends first (only needed once)
cd depends
make HOST=x86_64-w64-mingw32 -j$(nproc)
cd ..

# Configure and build
make distclean
CONFIG_SITE=$HOME/projects/rabidcoin/depends/x86_64-w64-mingw32/share/config.site \
./configure --prefix=/ --host=x86_64-w64-mingw32 --disable-tests --disable-bench \
--with-gui=qt5 --disable-hardening
make -j$(nproc) src/qt/rabidcoin-qt.exe
x86_64-w64-mingw32-strip src/qt/rabidcoin-qt.exe
```

## Build Windows CLI
```
make -j$(nproc) src/dogecoin-cli.exe
x86_64-w64-mingw32-strip src/dogecoin-cli.exe
cp src/dogecoin-cli.exe src/rabidcoin-cli.exe
```

## Build Linux Daemon
```
make -j$(nproc) src/rabidcoind
strip src/rabidcoind
```

## Stratum Server (Python, no build needed)
- Script: stratum-server.py (in repo root)
- Requires: pip install requests
- Windows: use bundled python/ folder from release zip

## Windows Bundle (embedded Python)
```
mkdir -p bundle/python
wget https://www.python.org/ftp/python/3.11.9/python-3.11.9-embed-amd64.zip
unzip python-3.11.9-embed-amd64.zip -d bundle/python
sed -i 's/#import site/import site/' bundle/python/python311._pth
mkdir -p bundle/python/Lib/site-packages
pip3 download requests --platform win_amd64 --python-version 311 --only-binary=:all: -d bundle/wheels
cd bundle/wheels && for whl in *.whl; do unzip -o "$whl" -d ../python/Lib/site-packages/; done
```

## VPS Setup (Ubuntu 24.04)
- Daemon: systemd service rabidcoin
- Stratum: systemd service rabidcoin-stratum  
- Explorer: systemd service rabidcoin-explorer (port 3001, nginx proxy)
- MongoDB: for explorer

## testnet rabidcoin.conf
```
testnet=1
server=1
rpcuser=rabiduser
rpcpassword=rabidpass123
rpcport=17332
port=17333
addnode=194.163.150.15
addnode=testnet-seed.rabidmining.com
listen=1
```

## GitHub Releases
- Coin releases: https://github.com/RabidMining/Rabid-Coin/releases
- Miner releases: https://github.com/RabidMining/xmrig-rabid/releases
