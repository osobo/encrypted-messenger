#ifndef CHACHA20_H__
#define CHACHA20_H__

#include <stdio.h>
#include "machine_defs.h"


typedef struct
{
  u32 words[16];
} ChaChaCtx;


// Applies 20 rounds (10 double rounds) using ctx as inp, stores res in out.
// ctx is not mutated in any way, ctr is _not_ increased.
void chacha20(const ChaChaCtx* ctx, u8 out[]);

// Sets all word in ctx to 0.
void chacha_set_zero(ChaChaCtx* ctx);

// Sets words 0..3 to the constants defined by the spec (same as salsa).
void chacha_set_consts(ChaChaCtx* ctx);

// nonce is assumed to be len 8 bytes.
void chacha_set_nonce(ChaChaCtx* ctx, const u8 nonce[]);

// The ctr is just 8 words = 256 bits
// key is assumed to be len 32 bytes.
void chacha_set_key(ChaChaCtx* ctx, /*const u32 key[]*/ const u8 key[]);

// Extract ctr from ctx as two u32s, a is least sig, b is most sig.
void chacha_extract_ctr(const ChaChaCtx* ctx, u32* a, u32* b);

void chacha_set_ctr(ChaChaCtx* ctx, u32 a, u32 b);

// Increases the ctr of ctx by 1.
void chacha_inc_ctr(ChaChaCtx* ctx);


// Sets the constant words in ctx correctly.
// Sets the key words in ctx according to the key arg.
// Sets the nonce words in ctx according to the nonce arg.
// Sets ctr in ctx to 0.
void chacha_init(ChaChaCtx* ctx, const u8 key[], const u8 nonce[]);


// Assumes len is multiple of 64.
void chacha20_keystream(ChaChaCtx* ctx, u32 len, u8 res[]);

// Encrypt (or decrypt, same operation) len bytes in 'in' and store res in 'out'.
// Sets ctr to 0 at start and sets back to 0 afterwards.
// TODO: Super stupid with the keystream...
void chacha20_enc(ChaChaCtx* ctx, const u8 in[], u32 len, u8 out[]);

#endif // CHACHA20_H__
