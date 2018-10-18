#include <stdio.h>
#include "dh.h"

static const MontgomeryCtx* ctx = &mctx;

// TODO: This is just a mock implementation. Implement for real on board using timers.
u32 random_word()
{
  static u32 a = 13;
  static u32 b = 101;
  static u32 x = 0x8EA38EA3;
  
  x = a*x + b;

  return x;
}

extern void rng_data(u32, void*);
// Generates a random private key of limb count R_LEN.
void dh_random_priv_key(DhPrivKey* priv)
{
  /*
  u32 i;
  for (i = 0; i < PRIV_LEN; ++i) {
    priv->limbs[i] = random_word();
  }
  */
  rng_data(sizeof(DhPrivKey), priv);
}


void dh_pub_key_to_bytes(const DhPubKey* pub, u8* bytes)
{
  Limb x[R_LEN];
  bigu_from_mont(pub->limbs, ctx, x);
  bigu_into_bytes(x, R_LEN, bytes);
}

void dh_bytes_to_pub_key(const u8* bytes, DhPubKey* pub)
{
  Limb x[R_LEN];
  bigu_from_bytes(bytes, R_LEN, x);
  bigu_into_mont(x, ctx, pub->limbs);
}


// Assumes priv and pub are of len R_LEN.
// Calcs the pub key given a priv key.
void dh_calc_pub_key(const DhPrivKey* priv, DhPubKey* pub)
{
  // pub = g ^ priv (mod n)
  bigu_mont_pow(gm, priv->limbs, PRIV_LEN, ctx, pub->limbs);
}

// Given a priv key and another party's pub key: calc shared secret.
// len of ss should obviously be R_LEN * LIMB_BYTE_COUNT.
void dh_calc_shared_secret(const DhPrivKey* priv, const DhPubKey* pub, u8* ss)
{
  Limb x[R_LEN];
  Limb y[R_LEN];

  // Perform actual calc. Store res in x.
  bigu_mont_pow(pub->limbs, priv->limbs, PRIV_LEN, ctx, x);
  // Res is in montgomery form. Covert out of it. Store new form in y.
  bigu_from_mont(x, ctx, y);
  // Write y to ss in byte array big endian form.
  bigu_into_bytes(y, R_LEN, ss);
}


