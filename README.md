<h1 align="center">
<img src="https://raw.githubusercontent.com/RabidMining/Rabid-Coin/master/share/pixmaps/rabidcoin256.png" alt="RabidCoin" width="256"/>
<br/><br/>
RabidCoin Core [RABID]
</h1>

RabidCoin is a community-driven cryptocurrency powered by the GhostRider proof-of-work algorithm. It is forked from Dogecoin Core and adapted for CPU mining with GhostRider, making it ASIC-resistant and accessible to everyone.

## Key Features

- **GhostRider PoW** - 15-algorithm rotating hash function, ASIC-resistant and CPU-friendly
- **2% Founder Fee** - Applied at block 5000+ to support ongoing development
- **Fast Blocks** - Inherited from Dogecoin's proven blockchain architecture
- **R Addresses** - All mainnet addresses start with `R`

## Specifications

| Parameter | Value |
|-----------|-------|
| Algorithm | GhostRider (15 sph hash functions) |
| Block Reward | 10,000 RABID (fixed forever, no halvings) |
| Founder Fee | 2% (block 5000+) |
| P2P Port | 7333 |
| RPC Port | 7332 |
| Address Prefix | R |
| Network Magic | RABI (0x52 0x41 0x42 0x49) |

## Genesis Block

| Parameter | Value |
|-----------|-------|
| Hash | `1eadba82b2a025e731d39838d0297a07fd8517a820713374b8fd1602cd38ad98` |
| Merkle Root | `b11e57390a123182404a9352fa2cb16dc82be6bf4ef219bbbeb03102a5cb2812` |
| Timestamp | `Rabid Mining launches Rabidcoin - GhostRider powered, Rabid to the moon!! @MiningRabid 2026` |
| nTime | 1743380000 |
| nNonce | 573888 |
| nBits | 0x1e0ffff0 |

## Ports

| Function | Mainnet | Testnet | Regtest |
|----------|---------|---------|---------|
| P2P | 7333 | 17333 | 18444 |
| RPC | 7332 | 17332 | 18332 |

## Building

### Linux
```bash
./autogen.sh
./configure --with-incompatible-bdb --disable-tests --disable-bench --without-gui
make -j$(nproc)
```

### With wallet support
```bash
./autogen.sh
BDB_CFLAGS="-I/usr/include" BDB_LIBS="-L/usr/lib/x86_64-linux-gnu -ldb_cxx" ./configure --with-incompatible-bdb --disable-tests --disable-bench --without-gui
make -j$(nproc)
```

## Running
```bash
# Start daemon
./src/dogecoind -daemon -rpcuser=youruser -rpcpassword=yourpass

# Check status
./src/rabidcoin-cli -rpcuser=youruser -rpcpassword=yourpass getblockchaininfo

# Generate address
./src/rabidcoin-cli -rpcuser=youruser -rpcpassword=yourpass getnewaddress
```

## Mining

RabidCoin uses the GhostRider algorithm. Use XMRig with GhostRider support:
```json
{
    "pools": [
        {
            "algo": "ghostrider",
            "url": "YOUR_NODE_IP:7332",
            "user": "YOUR_RABID_ADDRESS",
            "pass": "x",
            "daemon": true
        }
    ]
}
```

## Links

- GitHub: https://github.com/RabidMining/Rabid-Coin
- Twitter: @MiningRabid

## License

RabidCoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more information.

## Connecting to Testnet

Create a `rabidcoin.conf` file in your data directory:
```
testnet=1
addnode=194.163.150.15:17333
listen=0
```

Or launch with flags:
```
rabidcoin-qt.exe -testnet -addnode=194.163.150.15:17333 -listen=0
```

Testnet seed node: 194.163.150.15:17333
