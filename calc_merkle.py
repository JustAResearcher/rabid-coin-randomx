import hashlib
import struct
import binascii

psz_timestamp = b'Rabid Mining launches Rabidcoin - CPU/GhostRider powered, Rabid to the moon!! @MiningRabid 2026'
n_time = 1743380000
n_bits = 0x1e0ffff0
n_version = 1
genesis_reward = 88 * 100000000
pubkey_hex = '040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9'

coinbase_script = b'\x04\xff\xff\x00\x1d\x01\x04' + psz_timestamp
tx_in = b'\x00' * 32 + b'\xff\xff\xff\xff' + bytes([len(coinbase_script)]) + coinbase_script + b'\xff\xff\xff\xff'
script_pubkey = binascii.unhexlify(pubkey_hex) + b'\xac'
tx_out_value = struct.pack('<Q', genesis_reward)
tx_out = tx_out_value + bytes([len(script_pubkey)]) + script_pubkey
tx = struct.pack('<i', n_version) + b'\x01' + tx_in + b'\x01' + tx_out + b'\x00\x00\x00\x00'
tx_hash = hashlib.sha256(hashlib.sha256(tx).digest()).digest()[::-1]

print('Merkle Root:', binascii.hexlify(tx_hash).decode())
print('nTime:', n_time)
print('nBits:', hex(n_bits))