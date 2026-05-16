#!/usr/bin/env bash

function miner_config_gen() {
    # TODO(randomx): RabidCoin PoW is now RandomXv2 (rx/0). Stock xmrig works;
    # the legacy xmrig-rabid build is no longer needed.
    DIR="/hive/miners/custom/xmrig"
    POOL=${CUSTOM_POOL:-"stratum+tcp://stratum.rabidmining.com:3333"}
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
}
