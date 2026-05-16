# RabidCoin (RABID)

A Dogecoin fork using the RandomXv2 Proof-of-Work algorithm.

## Testnet Mining

### Pool
- **Stratum**: `stratum.rabidmining.com:3333`
- **Algorithm**: `rx/0` (RandomXv2)
- **Block Reward**: 10,000 RABID
- **Block Time**: ~60 seconds

### Miners
RabidCoin uses RandomXv2, so any RandomX-capable miner works (e.g. **xmrig**,
**srbminer-multi**). CPU mining is recommended.

- **xmrig**: <https://github.com/xmrig/xmrig/releases>
- **srbminer-multi**: <https://github.com/doktor83/SRBMiner-Multi/releases>

### Quick Start (Windows / Linux / HiveOS)
1. Download a recent build of `xmrig` (or another RandomX miner).
2. Point it at the pool with algo `rx/0`:

```bash
./xmrig -o stratum.rabidmining.com:3333 -a rx/0 -u YOUR_WALLET.WORKER -p x
```

### Quick Start (HiveOS)
1. Create Flight Sheet → Miner: `xmrig`
2. Pool: `stratum.rabidmining.com:3333`
3. Algo: `rx/0`
4. Wallet template: `YOUR_WALLET.%WORKER_NAME%`

## Testnet Node
- P2P: `194.163.150.15:17333`
- DNS Seed: `testnet-seed.rabidmining.com`

## Links
- [XMRig-Rabid Miner](https://github.com/RabidMining/xmrig-rabid)
- [RabidMining.com](https://rabidmining.com)
- Twitter: [@MiningRabid](https://twitter.com/MiningRabid)
