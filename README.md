# RabidCoin (RABID)

A Dogecoin fork with custom GR-Rabid Proof-of-Work algorithm.

## Testnet Mining

### Pool
- **Stratum**: `stratum.rabidmining.com:3333`
- **Algorithm**: `gr-rabid`
- **Block Reward**: 10,000 RABID
- **Block Time**: ~60 seconds

### Miners
- **Windows**: [xmrig-rabid-windows-v1.0.0.zip](https://github.com/RabidMining/xmrig-rabid/releases/download/v1.0.0-testnet/xmrig-rabid-windows-v1.0.0.zip)
- **HiveOS/Linux**: [xmrig-rabid-1.0.0.tar.gz](https://github.com/RabidMining/xmrig-rabid/releases/download/v1.0.0-testnet/xmrig-rabid-1.0.0.tar.gz)

### Quick Start (Windows)
1. Download and extract `xmrig-rabid-windows-v1.0.0.zip`
2. Edit `start-mining.bat` with your wallet address
3. Run `start-mining.bat`

### Quick Start (HiveOS)
1. Create Flight Sheet → Custom miner
2. Installation URL: `https://github.com/RabidMining/xmrig-rabid/releases/download/v1.0.0-testnet/xmrig-rabid-1.0.0.tar.gz`
3. Pool: `stratum.rabidmining.com:3333`
4. Algo: `gr-rabid`

### Quick Start (Linux)
```bash
./xmrig-rabid -o stratum.rabidmining.com:3333 -a gr-rabid -u YOUR_WALLET -p x
```

## Testnet Node
- P2P: `194.163.150.15:17333`
- DNS Seed: `testnet-seed.rabidmining.com`

## Links
- [XMRig-Rabid Miner](https://github.com/RabidMining/xmrig-rabid)
- [RabidMining.com](https://rabidmining.com)
- Twitter: [@MiningRabid](https://twitter.com/MiningRabid)
