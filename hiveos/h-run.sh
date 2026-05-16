#!/usr/bin/env bash
if ! ldconfig -p | grep -q libhwloc.so.15; then
    apt-get install -y libhwloc15 2>/dev/null
fi

# Source wallet config to get CUSTOM_TEMPLATE, CUSTOM_URL etc
source /hive-config/wallet.conf 2>/dev/null

# TODO(randomx): RabidCoin PoW is now RandomXv2 (rx/0). Stock xmrig handles rx/0
# natively; legacy xmrig-rabid binary is no longer required.
DIR="/hive/miners/custom/xmrig"
export LD_LIBRARY_PATH=$DIR/libs:$LD_LIBRARY_PATH
POOL=${CUSTOM_URL:-"stratum.rabidmining.com:3333"}
[[ "$POOL" != stratum+tcp://* ]] && POOL="stratum+tcp://$POOL"
WALLET=$(echo ${CUSTOM_TEMPLATE:-"nprxq6xT3kMdHiv8HXGvrPis33kxtUkxBy"} | cut -d'.' -f1)
WORKER=${CUSTOM_TEMPLATE#*.}
[[ "$WORKER" == "$WALLET" ]] && WORKER=${WORKER_NAME:-"worker1"}

cat > $DIR/config.json << EOJSON
{
    "autosave": false,
    "donate-level": 0,
    "api": { "port": 44444 },
    "pools": [{
        "url": "$POOL",
        "user": "$WALLET.$WORKER",
        "pass": "x",
        "algo": "rx/0",
        "keepalive": true,
        "nicehash": false,
        "tls": false
    }],
    "cpu": {
        "enabled": true,
        "huge-pages": true,
        "hw-aes": null,
        "priority": 2
    }
}
EOJSON

exec $DIR/xmrig --config=$DIR/config.json $CUSTOM_USER_CONFIG
