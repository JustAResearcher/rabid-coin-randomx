#!/usr/bin/env python3
import socket, json, threading, requests, hashlib, struct, time, binascii, os
import calendar

RPC_URL = "http://rabiduser:rabidpass123@127.0.0.1:17332"
STRATUM_PORT = 3333
COINBASE_ADDR = "nprxq6xT3kMdHiv8HXGvrPis33kxtUkxBy"

# Vardiff settings
VARDIFF_TARGET_SHARES_PER_MIN = 4      # target 1 share per 10 seconds
VARDIFF_RETARGET_EVERY = 10            # retarget every 10 shares
VARDIFF_MIN_DIFF = 10
VARDIFF_MAX_DIFF = 100000

# Auto-payout settings
PAYOUT_ENABLED = True
PAYOUT_MIN = 100  # minimum RABID to trigger payout (in full coins)

def rpc(method, params=[]):
    r = requests.post(RPC_URL, json={"jsonrpc":"1.0","id":"1","method":method,"params":params}, timeout=10)
    return r.json()

def dsha256(data):
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()

def build_coinbase_split(template, miner_addr=COINBASE_ADDR):
    height = template['height']
    value = template['coinbasevalue']
    r = rpc("validateaddress", [miner_addr])
    script = r['result']['scriptPubKey']

    if height < 0xfd:
        h = struct.pack('<B', height)
    elif height <= 0xffff:
        h = b'\xfd' + struct.pack('<H', height)
    else:
        h = b'\xfe' + struct.pack('<I', height)

    height_script = bytes([len(h)]) + h
    tag = b'RabidCoin'
    scriptsig_prefix = height_script + tag
    total_script_len = len(scriptsig_prefix) + 4 + 4

    cb1 = bytes.fromhex('01000000')
    cb1 += bytes([1])
    cb1 += bytes(32)
    cb1 += bytes.fromhex('ffffffff')
    cb1 += bytes([total_script_len]) + scriptsig_prefix

    cb2 = bytes.fromhex('ffffffff')
    cb2 += bytes([1])
    cb2 += struct.pack('<q', value)
    cb2 += bytes([len(script)//2]) + bytes.fromhex(script)
    cb2 += bytes(4)

    return cb1.hex(), cb2.hex()

def merkle_root(coinbase_hash, branch):
    root = coinbase_hash
    for b in branch:
        data = bytes.fromhex(root) + bytes.fromhex(b)
        root = dsha256(data).hex()
    return root

jobs = {}
jobs_lock = threading.Lock()

def make_job(template, miner_addr=COINBASE_ADDR):
    job_id = binascii.hexlify(os.urandom(4)).decode()
    prevhash = bytes.fromhex(template['previousblockhash'])[::-1].hex()
    version = struct.pack('<I', template['version']).hex()
    nbits = struct.pack('<I', int(template['bits'], 16)).hex()
    ntime = struct.pack('<I', template['curtime']).hex()
    coinb1, coinb2 = build_coinbase_split(template, miner_addr)
    merkle_branch = [t['hash'] for t in template.get('transactions', [])]
    with jobs_lock:
        jobs[job_id] = {
            'template': template,
            'coinb1': coinb1,
            'coinb2': coinb2,
            'miner_addr': miner_addr,
            'merkle_branch': merkle_branch,
            'version': version,
            'nbits': nbits,
            'ntime': ntime,
            'prevhash': prevhash
        }
    return job_id, [job_id, prevhash, coinb1, coinb2, merkle_branch, version, nbits, ntime, True]

def submit_block(job_id, extranonce1, extranonce2, ntime, nonce):
    with jobs_lock:
        job = jobs.get(job_id)
    if not job:
        return False

    cb_hex = job['coinb1'] + extranonce1 + extranonce2 + job['coinb2']
    cb_hash = dsha256(bytes.fromhex(cb_hex)).hex()
    mr = merkle_root(cb_hash, job['merkle_branch'])

    header = struct.pack('<I', job['template']['version']).hex()
    header += bytes.fromhex(job['template']['previousblockhash'])[::-1].hex()
    header += mr
    header += ntime
    header += struct.pack('<I', int(job['template']['bits'], 16)).hex()
    header += struct.pack('<I', int(nonce, 16)).hex()

    template = job['template']
    txs = template.get('transactions', [])
    tx_count = len(txs) + 1
    if tx_count < 0xfd:
        block = header + '%02x' % tx_count
    else:
        block = header + 'fd' + '%04x' % tx_count
    block += cb_hex
    for tx in txs:
        block += tx['data']

    result = rpc("submitblock", [block])
    success = result.get('result') is None and result.get('error') is None
    print(f"submitblock: {result.get('result')} error={result.get('error')}")
    return success, job['miner_addr'], job['template']['coinbasevalue']

# Worker tracking
workers = {}  # addr -> {shares, hashrate, last_share_time, diff, share_times, pending_payout}
workers_lock = threading.Lock()

def get_worker(addr):
    with workers_lock:
        if addr not in workers:
            workers[addr] = {
                'shares': 0,
                'hashrate': 0,
                'last_share_time': time.time(),
                'diff': 10,
                'share_times': [],
                'pending_payout': 0.0,
                'total_earned': 0.0,
                'blocks_found': 0,
                'connected': False,
                'extranonce1': None
            }
        return workers[addr]

def update_vardiff(addr):
    """Adjust difficulty based on share rate"""
    with workers_lock:
        w = workers.get(addr)
        if not w:
            return w['diff'] if w else 10

        now = time.time()
        # Keep only last VARDIFF_RETARGET_EVERY share times
        w['share_times'].append(now)
        if len(w['share_times']) > VARDIFF_RETARGET_EVERY:
            w['share_times'] = w['share_times'][-VARDIFF_RETARGET_EVERY:]

        if len(w['share_times']) < 2:
            return w['diff']

        # Calculate actual shares per minute
        elapsed = w['share_times'][-1] - w['share_times'][0]
        if elapsed <= 0:
            return w['diff']

        actual_shares_per_min = (len(w['share_times']) - 1) / elapsed * 60
        target = VARDIFF_TARGET_SHARES_PER_MIN

        # Adjust difficulty
        if actual_shares_per_min > target * 1.5:
            new_diff = int(w['diff'] * 1.5)
        elif actual_shares_per_min < target * 0.5:
            new_diff = max(int(w['diff'] * 0.75), VARDIFF_MIN_DIFF)
        else:
            return w['diff']

        new_diff = max(VARDIFF_MIN_DIFF, min(VARDIFF_MAX_DIFF, new_diff))
        if new_diff != w['diff']:
            print(f"Vardiff {addr}: {w['diff']} -> {new_diff} (rate={actual_shares_per_min:.1f}/min)")
            w['diff'] = new_diff
        return w['diff']

def update_hashrate(addr, diff):
    """Estimate hashrate from difficulty and share time"""
    with workers_lock:
        w = workers.get(addr)
        if not w or len(w['share_times']) < 2:
            return
        elapsed = w['share_times'][-1] - w['share_times'][0]
        if elapsed <= 0:
            return
        shares = len(w['share_times']) - 1
        # hashrate = diff * 2^32 / avg_share_time
        avg_time = elapsed / shares
        w['hashrate'] = int(diff * 4294967296 / avg_time)

def do_payout(block_reward_satoshis, winning_addr):
    """Split block reward among all workers by share contribution"""
    if not PAYOUT_ENABLED:
        return

    with workers_lock:
        total_shares = sum(w['shares'] for w in workers.values() if w['shares'] > 0)
        if total_shares == 0:
            return

        block_reward = block_reward_satoshis / 1e8  # convert to coins
        print(f"\n=== BLOCK FOUND! Reward: {block_reward} RABID, Total shares: {total_shares} ===")

        payouts = {}
        for addr, w in workers.items():
            if w['shares'] > 0:
                share = w['shares'] / total_shares
                amount = block_reward * share
                w['pending_payout'] += amount
                w['total_earned'] += amount
                w['blocks_found'] += 1
                payouts[addr] = amount
                print(f"  {addr}: {w['shares']} shares ({share*100:.1f}%) = {amount:.2f} RABID (pending: {w['pending_payout']:.2f})")

        # Reset shares for next round
        for w in workers.values():
            w['shares'] = 0

    # Send payouts
    for addr, amount in payouts.items():
        with workers_lock:
            pending = workers[addr]['pending_payout']

        if pending >= PAYOUT_MIN and addr != COINBASE_ADDR:
            try:
                # Check wallet balance first
                balance = rpc("getbalance").get('result', 0)
                if balance and float(balance) >= pending:
                    result = rpc("sendtoaddress", [addr, round(pending, 4)])
                    if result.get('result'):
                        txid = result['result']
                        print(f"PAYOUT: {pending:.4f} RABID -> {addr} txid={txid}")
                        with workers_lock:
                            workers[addr]['pending_payout'] = 0.0
                    else:
                        print(f"Payout failed for {addr}: {result.get('error')}")
                else:
                    print(f"Insufficient balance for payout to {addr} (need {pending:.4f}, have {balance})")
            except Exception as e:
                print(f"Payout error for {addr}: {e}")

def get_stats():
    """Get current pool stats for dashboard"""
    with workers_lock:
        stats = []
        for addr, w in workers.items():
            if w['connected']:
                hr = w['hashrate']
                if hr > 1000000:
                    hr_str = f"{hr/1000000:.2f} MH/s"
                elif hr > 1000:
                    hr_str = f"{hr/1000:.2f} KH/s"
                else:
                    hr_str = f"{hr} H/s"
                stats.append({
                    'addr': addr,
                    'shares': w['shares'],
                    'hashrate': hr_str,
                    'pending': round(w['pending_payout'], 4),
                    'total_earned': round(w['total_earned'], 4),
                    'diff': w['diff']
                })
        return stats

clients = []
clients_lock = threading.Lock()

def send_msg(conn, msg):
    try:
        conn.send((json.dumps(msg) + "\n").encode())
    except:
        pass

def handle_client(conn, addr):
    print(f"Miner connected: {addr}")
    buf = ""
    conn.settimeout(600)
    extranonce1 = binascii.hexlify(os.urandom(4)).decode()
    miner_addr = [COINBASE_ADDR]
    try:
        while True:
            data = conn.recv(4096).decode()
            if not data:
                break
            buf += data
            while "\n" in buf:
                line, buf = buf.split("\n", 1)
                if not line.strip():
                    continue
                try:
                    msg = json.loads(line)
                    handle_message(conn, msg, extranonce1, miner_addr)
                except Exception as e:
                    print(f"Error: {e}")
    except Exception as e:
        print(f"Disconnected {addr}: {e}")
    finally:
        with workers_lock:
            if miner_addr[0] in workers:
                workers[miner_addr[0]]['connected'] = False
        with clients_lock:
            clients[:] = [(c,a) for c,a in clients if c != conn]
        conn.close()

def handle_message(conn, msg, extranonce1, miner_addr):
    method = msg.get("method","")
    mid = msg.get("id")
    params = msg.get("params", [])

    if method == "mining.subscribe":
        send_msg(conn, {"id":mid,"result":[[["mining.set_difficulty","1"],["mining.notify","1"]], extranonce1, 4],"error":None})
        send_msg(conn, {"id":None,"method":"mining.set_difficulty","params":[10]})

    elif method == "mining.authorize":
        addr = params[0].split(".")[0] if params else COINBASE_ADDR
        miner_addr[0] = addr
        print(f"Authorized: {addr}")
        send_msg(conn, {"id":mid,"result":True,"error":None})

        # Init worker
        w = get_worker(addr)
        with workers_lock:
            workers[addr]['connected'] = True
            workers[addr]['extranonce1'] = extranonce1

        with clients_lock:
            clients[:] = [(c,a) for c,a in clients if c != conn]
            clients.append((conn, addr))

        # Send initial difficulty
        with workers_lock:
            diff = workers[addr]['diff']
        send_msg(conn, {"id":None,"method":"mining.set_difficulty","params":[diff]})

        tmpl = rpc("getblocktemplate", [{"rules":["segwit"]}]).get("result")
        if tmpl:
            _, job = make_job(tmpl, addr)
            send_msg(conn, {"id":None,"method":"mining.notify","params":job})

    elif method == "mining.submit":
        if len(params) >= 5:
            worker, job_id, extranonce2, ntime, nonce = params[:5]
            addr = miner_addr[0]

            # Track share
            w = get_worker(addr)
            with workers_lock:
                workers[addr]['shares'] += 1
                workers[addr]['last_share_time'] = time.time()

            # Vardiff
            new_diff = update_vardiff(addr)
            with workers_lock:
                old_diff = workers[addr]['diff']
            if new_diff != old_diff:
                send_msg(conn, {"id":None,"method":"mining.set_difficulty","params":[new_diff]})

            # Update hashrate estimate
            update_hashrate(addr, new_diff)

            # Try submit block
            result = submit_block(job_id, extranonce1, extranonce2, ntime, nonce)
            if result and result[0]:
                # Block found!
                _, winning_addr, coinbase_value = result
                print(f"\n🐺 BLOCK FOUND by {addr}! Reward: {coinbase_value/1e8:.0f} RABID")
                threading.Thread(target=do_payout, args=(coinbase_value, winning_addr), daemon=True).start()

        send_msg(conn, {"id":mid,"result":True,"error":None})

    elif method == "mining.extranonce.subscribe":
        send_msg(conn, {"id":mid,"result":True,"error":None})

def job_broadcaster():
    last_hash = None
    while True:
        try:
            tmpl = rpc("getblocktemplate", [{"rules":["segwit"]}]).get("result")
            if tmpl and tmpl['previousblockhash'] != last_hash:
                last_hash = tmpl['previousblockhash']
                with clients_lock:
                    connected = clients[:]
                print(f"New job height={tmpl['height']} miners={len(connected)}")
                # Print stats
                stats = get_stats()
                for s in stats:
                    print(f"  Worker {s['addr'][:16]}... diff={s['diff']} shares={s['shares']} hr={s['hashrate']} pending={s['pending']} RABID")
                for item in connected:
                    try:
                        c, addr = item
                        _, pjob = make_job(tmpl, addr)
                        send_msg(c, {"id":None,"method":"mining.notify","params":pjob})
                    except:
                        pass
        except Exception as e:
            print(f"Broadcaster: {e}")
        time.sleep(5)

server = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
server.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("::", STRATUM_PORT))
server.listen(50)
print(f"RabidCoin stratum on port {STRATUM_PORT}")
print(f"Vardiff: target {VARDIFF_TARGET_SHARES_PER_MIN} shares/min, retarget every {VARDIFF_RETARGET_EVERY} shares")
print(f"Auto-payout: {'ENABLED' if PAYOUT_ENABLED else 'DISABLED'} (min {PAYOUT_MIN} RABID)")
t = threading.Thread(target=job_broadcaster, daemon=True)
t.start()
while True:
    conn, addr = server.accept()
    t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
    t.start()
