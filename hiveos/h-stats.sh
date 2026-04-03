#!/usr/bin/env bash
API_PORT=44444
STATS=$(curl -s --connect-timeout 2 http://127.0.0.1:$API_PORT/2/summary)
if [[ -z $STATS ]]; then
    echo '{"hs":[],"hs_units":"hs","temp":[],"fan":[],"uptime":0,"ar":[0,0],"algo":"gr-rabid"}'
    exit
fi

HASHRATE=$(echo $STATS | jq -r '.hashrate.total[0] // 0')
ACCEPTED=$(echo $STATS | jq -r '.results.shares_good // 0')
REJECTED=$(echo $STATS | jq -r '(.results.shares_total // 0) - (.results.shares_good // 0)')
UPTIME=$(echo $STATS | jq -r '.uptime // 0')
ALGO=$(echo $STATS | jq -r '.algo // "gr-rabid"')

cat << EOJSON
{"hs":[$HASHRATE],"hs_units":"hs","temp":[],"fan":[],"uptime":$UPTIME,"ar":[$ACCEPTED,$REJECTED],"algo":"$ALGO"}
EOJSON
