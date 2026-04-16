#!/usr/bin/env python3
import socket, json, threading, requests, hashlib, struct, time, binascii, os
import calendar

RPC_URL = "http://rabiduser:rabidpass123@127.0.0.1:17332"
STRATUM_PORT = 3333
COINBASE_ADDR = "nprxq6xT3kMdHiv8HXGvrPis33kxtUkxBy"

def rpc(method, params=[]):
    r = requests.post(RPC_URL, json={"jsonrpc":"1.0","id":"1","method":method,"params":params}, timeout=10)
    return r.json()

def dsha256(data):
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()

def build_coinbase(template, extranonce1, extranonce2):
    height = template['height']
    value = template['coinbasevalue']
    r = rpc("validateaddress", [COINBASE_ADDR])
    script = r['result']['scriptPubKey']
    
    # Height script
    if height < 0xfd:
        h = struct.pack('<B', height)
    elif height <= 0xffff:
        h = b'\xfd' + struct.pack('<H', height)
    else:
        h = b'\xfe' + struct.pack('<I', height)
    
    height_script = bytes([len(h)]) + h
    tag = b'RabidCoin'
    
    # Full coinbase scriptSig with extranonces embedded
    scriptsig = height_script + tag + bytes.fromhex(extranonce1) + bytes.fromhex(extranonce2)
    
    cb = bytes.fromhex('01000000')  # version
    cb += bytes([1])  # input count
    cb += bytes(32)   # prevout hash
    cb += bytes.fromhex('ffffffff')  # prevout index
    cb += bytes([len(scriptsig)]) + scriptsig  # scriptSig
    cb += bytes.fromhex('ffffffff')  # sequence
    cb += bytes([1])  # output count
    cb += struct.pack('<q', value)  # value
    cb += bytes([len(script)//2]) + bytes.fromhex(script)  # scriptPubKey
    cb += bytes(4)  # locktime
    
    return cb.hex()

def build_coinbase_split(template, miner_addr=COINBASE_ADDR):
    """Build coinbase split at extranonce position"""
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
    
    # coinb1: everything up to extranonce
    cb1 = bytes.fromhex('01000000')  # version
    cb1 += bytes([1])   # input count
    cb1 += bytes(32)    # prevout hash
    cb1 += bytes.fromhex('ffffffff')  # prevout index
    # script length placeholder - we'll include height+tag before extranonce
    scriptsig_prefix = height_script + tag
    # total script length = prefix + extranonce1(4) + extranonce2(4) 
    total_script_len = len(scriptsig_prefix) + 4 + 4
    cb1 += bytes([total_script_len]) + scriptsig_prefix
    
    # coinb2: after extranonce2
    FOUNDER_ADDR = "neZq4JxrsC2H8CVprngCazESVN86GBpsY3"
    founder_script = rpc("validateaddress", [FOUNDER_ADDR])['result']['scriptPubKey']
    founder_fee = value // 50  # 2%
    miner_value = value - founder_fee
    use_founder = height >= 5000
    cb2 = bytes.fromhex('ffffffff')  # sequence
    cb2 += bytes([2 if use_founder else 1])  # output count
    cb2 += struct.pack('<q', miner_value if use_founder else value)  # miner value
    cb2 += bytes([len(script)//2]) + bytes.fromhex(script)
    if use_founder:
        cb2 += struct.pack('<q', founder_fee)  # founder value
        cb2 += bytes([len(founder_script)//2]) + bytes.fromhex(founder_script)
    cb2 += bytes(4)  # locktime
    
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
    # Send values in node LE format (XMRig GR_RABID doesn't swap)
    prevhash = bytes.fromhex(template['previousblockhash'])[::-1].hex()
    version = struct.pack('<I', template['version']).hex()
    nbits = struct.pack('<I', int(template['bits'], 16)).hex()
    ntime = struct.pack('<I', template['curtime']).hex()  # LE for correct storage
    coinb1, coinb2 = build_coinbase_split(template, miner_addr)
    txids = [t.get('txid') or t.get('hash') for t in template.get('transactions', [])]
    def get_branch(txids_be):
        branch = []
        layer = [bytes.fromhex(t)[::-1] for t in txids_be]
        index = 0
        while len(layer) > 1:
            if len(layer) % 2 == 1:
                layer.append(layer[-1])
            sibling = index ^ 1
            branch.append(layer[sibling].hex())  # sibling LE
            layer = [dsha256(layer[i]+layer[i+1]) for i in range(0, len(layer), 2)]
            index //= 2
        return branch
    merkle_branch = get_branch([bytes(32).hex()] + txids) if txids else []
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
    print("JOB coinb1="+coinb1[:40]+" coinb2="+coinb2[:40])
    return job_id, [job_id, prevhash, coinb1, coinb2, merkle_branch, version, nbits, ntime, True]

def submit_block(job_id, extranonce1, extranonce2, ntime, nonce):
    print(f'DEBUG: en1={extranonce1} en2={extranonce2} ntime={ntime} nonce={nonce}')
    print(f'DEBUG: en1={extranonce1} en2={extranonce2} ntime={ntime} nonce={nonce}')
    with jobs_lock:
        job = jobs.get(job_id)
    if not job:
        return False
    
    # Build full coinbase
    cb_hex = job['coinb1'] + extranonce1 + extranonce2 + job['coinb2']
    cb_hash = dsha256(bytes.fromhex(cb_hex)).hex()  # raw dsha256 = LE merkle root
    
    # Calculate merkle root
    mr = merkle_root(cb_hash, job['merkle_branch'])
    
    # Build block header
    header = struct.pack('<I', job['template']['version']).hex()  # version LE
    # prevhash: reverse RPC display to get node LE bytes
    header += bytes.fromhex(job['template']['previousblockhash'])[::-1].hex()
    header += mr
    header += ntime  # ntime as raw hex (big-endian bytes as sent by XMRig)
    header += struct.pack('<I', int(job['template']['bits'], 16)).hex()  # nbits LE
    # nonce from XMRig is big-endian, reverse to little-endian
    # nonce: XMRig sends as BE hex, store as LE in header
    header += struct.pack('<I', int(nonce, 16)).hex()
    
    print(f"Header ({len(header)}): {header} nonce_submitted={nonce} nonce_in_header={header[152:160]}")
    
    # Build full block
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
    print(f"submitblock: {result.get('result')} error={result.get('error')}")
    return result.get('result') is None and result.get('error') is None

clients = []  # list of (conn, miner_addr) tuples
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
        with clients_lock:
            clients[:] = [(c,a) for c,a in clients if c != conn]
            clients.append((conn, addr))
        tmpl = rpc("getblocktemplate", [{"rules":["segwit"]}]).get("result")
        if tmpl:
            _, job = make_job(tmpl, addr)
            send_msg(conn, {"id":None,"method":"mining.notify","params":job})
    elif method == "mining.submit":
        print(f"SUBMIT params={params}")
        if len(params) >= 5:
            worker, job_id, extranonce2, ntime, nonce = params[:5]
            submit_block(job_id, extranonce1, extranonce2, ntime, nonce)
        send_msg(conn, {"id":mid,"result":True,"error":None})
    elif method == "mining.extranonce.subscribe":
        send_msg(conn, {"id":mid,"result":True,"error":None})

def job_broadcaster():
    last_hash = None
    last_refresh = 0
    while True:
        try:
            tmpl = rpc("getblocktemplate", [{"rules":["segwit"]}]).get("result")
            now = time.time()
            new_block = tmpl and tmpl['previousblockhash'] != last_hash
            stale_job = (now - last_refresh) > 60
            if tmpl and (new_block or stale_job):
                if new_block:
                    last_hash = tmpl['previousblockhash']
                last_refresh = now
                with clients_lock:
                    for item in clients[:]:
                        try:
                            c, addr = item if isinstance(item, tuple) else (item, COINBASE_ADDR)
                            _, pjob = make_job(tmpl, addr)
                            send_msg(c, {"id":None,"method":"mining.notify","params":pjob})
                        except: pass
                print(f"New job height={tmpl['height']} miners={len(clients)}")
        except Exception as e:
            print(f"Broadcaster: {e}")
        time.sleep(5)

server = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
server.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("::", STRATUM_PORT))
server.listen(50)
print(f"RabidCoin stratum on port {STRATUM_PORT}")
t = threading.Thread(target=job_broadcaster, daemon=True)
t.start()
while True:
    conn, addr = server.accept()
    t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
    t.start()
