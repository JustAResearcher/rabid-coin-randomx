#include "gr_hash.h"
#include <string.h>

extern "C" {
#include "../ghostrider/sph_blake.h"
#include "../ghostrider/sph_bmw.h"
#include "../ghostrider/sph_groestl.h"
#include "../ghostrider/sph_jh.h"
#include "../ghostrider/sph_keccak.h"
#include "../ghostrider/sph_skein.h"
#include "../ghostrider/sph_luffa.h"
#include "../ghostrider/sph_cubehash.h"
#include "../ghostrider/sph_shavite.h"
#include "../ghostrider/sph_simd.h"
#include "../ghostrider/sph_echo.h"
#include "../ghostrider/sph_hamsi.h"
#include "../ghostrider/sph_fugue.h"
#include "../ghostrider/sph_shabal.h"
#include "../ghostrider/sph_whirlpool.h"
}

static void h0(const uint8_t* in, size_t sz, uint8_t* out) { sph_blake512_context ctx; sph_blake512_init(&ctx); sph_blake512(&ctx, in, sz); sph_blake512_close(&ctx, out); }
static void h1(const uint8_t* in, size_t sz, uint8_t* out) { sph_bmw512_context ctx; sph_bmw512_init(&ctx); sph_bmw512(&ctx, in, sz); sph_bmw512_close(&ctx, out); }
static void h2(const uint8_t* in, size_t sz, uint8_t* out) { sph_groestl512_context ctx; sph_groestl512_init(&ctx); sph_groestl512(&ctx, in, sz); sph_groestl512_close(&ctx, out); }
static void h3(const uint8_t* in, size_t sz, uint8_t* out) { sph_jh512_context ctx; sph_jh512_init(&ctx); sph_jh512(&ctx, in, sz); sph_jh512_close(&ctx, out); }
static void h4(const uint8_t* in, size_t sz, uint8_t* out) { sph_keccak512_context ctx; sph_keccak512_init(&ctx); sph_keccak512(&ctx, in, sz); sph_keccak512_close(&ctx, out); }
static void h5(const uint8_t* in, size_t sz, uint8_t* out) { sph_skein512_context ctx; sph_skein512_init(&ctx); sph_skein512(&ctx, in, sz); sph_skein512_close(&ctx, out); }
static void h6(const uint8_t* in, size_t sz, uint8_t* out) { sph_luffa512_context ctx; sph_luffa512_init(&ctx); sph_luffa512(&ctx, in, sz); sph_luffa512_close(&ctx, out); }
static void h7(const uint8_t* in, size_t sz, uint8_t* out) { sph_cubehash512_context ctx; sph_cubehash512_init(&ctx); sph_cubehash512(&ctx, in, sz); sph_cubehash512_close(&ctx, out); }
static void h8(const uint8_t* in, size_t sz, uint8_t* out) { sph_shavite512_context ctx; sph_shavite512_init(&ctx); sph_shavite512(&ctx, in, sz); sph_shavite512_close(&ctx, out); }
static void h9(const uint8_t* in, size_t sz, uint8_t* out) { sph_simd512_context ctx; sph_simd512_init(&ctx); sph_simd512(&ctx, in, sz); sph_simd512_close(&ctx, out); }
static void h10(const uint8_t* in, size_t sz, uint8_t* out) { sph_echo512_context ctx; sph_echo512_init(&ctx); sph_echo512(&ctx, in, sz); sph_echo512_close(&ctx, out); }
static void h11(const uint8_t* in, size_t sz, uint8_t* out) { sph_hamsi512_context ctx; sph_hamsi512_init(&ctx); sph_hamsi512(&ctx, in, sz); sph_hamsi512_close(&ctx, out); }
static void h12(const uint8_t* in, size_t sz, uint8_t* out) { sph_fugue512_context ctx; sph_fugue512_init(&ctx); sph_fugue512(&ctx, in, sz); sph_fugue512_close(&ctx, out); }
static void h13(const uint8_t* in, size_t sz, uint8_t* out) { sph_shabal512_context ctx; sph_shabal512_init(&ctx); sph_shabal512(&ctx, in, sz); sph_shabal512_close(&ctx, out); }
static void h14(const uint8_t* in, size_t sz, uint8_t* out) { sph_whirlpool_context ctx; sph_whirlpool_init(&ctx); sph_whirlpool(&ctx, in, sz); sph_whirlpool_close(&ctx, out); }

typedef void (*core_hash_func)(const uint8_t*, size_t, uint8_t*);
static const core_hash_func core_hash[15] = { h0, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11, h12, h13, h14 };

static void select_indices(uint32_t* indices, size_t n, const uint8_t* seed)
{
    bool selected[15] = {};
    uint32_t k = 0;
    for (uint32_t i = 0; i < 64; ++i) {
        const uint8_t index = ((seed[i / 2] >> ((i & 1) * 4)) & 0xF) % n;
        if (!selected[index]) {
            selected[index] = true;
            indices[k++] = index;
            if (k >= n) return;
        }
    }
    for (uint32_t i = 0; i < n; ++i) {
        if (!selected[i]) {
            indices[k++] = i;
            if (k >= n) return;
        }
    }
}

uint256 GhostriderHash(const uint8_t* input, size_t len)
{
    uint8_t hash[64];
    uint8_t tmp[64];
    h4(input, len, hash);
    uint32_t indices[5];
    select_indices(indices, 5, hash);
    core_hash[indices[0]](input, len, hash);
    for (int i = 1; i < 5; i++) {
        core_hash[indices[i]](hash, 64, tmp);
        memcpy(hash, tmp, 64);
    }
    uint256 result;
    memcpy(&result, hash, 32);
    return result;
}
