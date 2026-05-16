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


def _nbits_to_target(n_bits):
    """Decode compact nBits (Bitcoin-style) into a 256-bit integer target."""
    exponent = n_bits >> 24
    mantissa = n_bits & 0x007fffff
    if exponent <= 3:
        target = mantissa >> (8 * (3 - exponent))
    else:
        target = mantissa << (8 * (exponent - 3))
    return target


def _serialize_header(nVersion, hashPrevBlock, hashMerkleRoot, nTime, nBits, nNonce):
    """Serialize an 80-byte block header in standard little-endian wire format.

    hashPrevBlock and hashMerkleRoot must be 32-byte little-endian byte strings.
    """
    assert len(hashPrevBlock) == 32
    assert len(hashMerkleRoot) == 32
    return (
        struct.pack('<i', nVersion)
        + hashPrevBlock
        + hashMerkleRoot
        + struct.pack('<I', nTime)
        + struct.pack('<I', nBits)
        + struct.pack('<I', nNonce)
    )


def mine_randomx_genesis(nVersion, hashPrevBlock, hashMerkleRoot, nTime, nBits):
    """Iterate nNonce until RandomXV2Hash(0, header_bytes, 80) < target.

    Returns (nNonce, hash_bytes) on success. Caller must wire RandomXV2Hash
    into the marked placeholder below — see the RandomX Python binding.

    hashPrevBlock and hashMerkleRoot are expected as 32-byte little-endian
    byte strings (the same form that goes on the wire).
    """
    target = _nbits_to_target(nBits)
    nNonce = 0
    while nNonce < 0x100000000:
        header_bytes = _serialize_header(
            nVersion, hashPrevBlock, hashMerkleRoot, nTime, nBits, nNonce
        )
        # TODO: call into RandomX binding
        # h = RandomXV2Hash(0, header_bytes, 80)
        # The binding is expected to return a 32-byte digest; PoW comparison
        # treats it as a little-endian 256-bit integer (Bitcoin convention).
        h = None
        if h is not None:
            h_int = int.from_bytes(h, 'little')
            if h_int < target:
                return nNonce, h
        if nNonce % 100000 == 0:
            print('mining... nNonce =', nNonce)
        nNonce += 1
    raise RuntimeError('exhausted 32-bit nNonce space without finding a valid hash')


if __name__ == '__main__':
    import sys
    if '--mine-genesis' in sys.argv:
        # tx_hash printed above is big-endian display form; on the wire the
        # merkle root is little-endian, which is what _serialize_header expects.
        merkle_root_le = hashlib.sha256(hashlib.sha256(tx).digest()).digest()
        prev_block_le = b'\x00' * 32
        print('Mining RandomXv2 genesis nNonce...')
        try:
            found_nonce, found_hash = mine_randomx_genesis(
                n_version, prev_block_le, merkle_root_le, n_time, n_bits
            )
            print('nNonce:', found_nonce)
            print('hash:', binascii.hexlify(found_hash[::-1]).decode())
        except RuntimeError as e:
            print('mining failed:', e)
            sys.exit(1)