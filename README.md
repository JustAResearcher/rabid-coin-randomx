# RabidCoin (RABID)

RabidCoin is a Proof-of-Work cryptocurrency created by
[Rabidminer](https://www.youtube.com/c/RabidMining). Starting at mainnet
block **5,775,000** (~2026-06-12), the chain mines with **RandomXv2** — the
same algorithm family as Monero — so it runs on commodity CPUs with no
specialised hardware required.

## Mining

### Pool
- **Stratum**: `stratum.rabidmining.com:3333`
- **Algorithm**: `rx/0` (RandomXv2)
- **Block Reward**: 10,000 RABID
- **Block Time**: ~60 seconds

### Miners
Any RandomX-capable miner works — **xmrig** and **srbminer-multi** are the
most common choices.

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

## RandomXv2 activation

Pre-activation, the chain runs the legacy GhostRider Proof-of-Work. At
mainnet block **5,775,000**, the chain switches to RandomXv2 from that
block onward. Auxiliary Proof-of-Work (merge-mining) is disabled at the
same height to prevent legacy-algorithm hashrate from bypassing the new
requirement via a synthesised parent header. Miners and pool operators
should be ready to run an xmrig-compatible setup before the activation
height is reached.

Testnet activates at block 5,910,000; regtest does not activate (stays on
GhostRider for fast local testing).

## Testnet Node
- P2P: `194.163.150.15:17333`
- DNS Seed: `testnet-seed.rabidmining.com`

## Links
- YouTube: [Rabidminer](https://www.youtube.com/c/RabidMining)
- Web: <https://rabidmining.com>
- Twitter: [@MiningRabid](https://twitter.com/MiningRabid)
