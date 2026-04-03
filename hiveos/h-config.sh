#!/usr/bin/env bash

function miner_config_gen() {
    DIR="/hive/miners/custom/xmrig-rabid"
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
        "algo": "gr-rabid",
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
