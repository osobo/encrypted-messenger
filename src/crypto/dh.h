#ifndef DH_H__
#define DH_H__

#include "bigu.h"
#include "dh-params.h"

// A bigu in montgomery form.
typedef struct
{
  Limb limbs[R_LEN];
} DhPubKey;

// A bigu, not in montgomery form.
typedef struct
{
  Limb limbs[PRIV_LEN];
} DhPrivKey;

// Generates a random private key of limb count R_LEN.
void dh_random_priv_key(DhPrivKey* priv);

void dh_pub_key_to_bytes(const DhPubKey* pub, u8* bytes);

void dh_bytes_to_pub_key(const u8* bytes, DhPubKey* pub);

// Assumes priv and pub are of len R_LEN.
// Calcs the pub key given a priv key.
void dh_calc_pub_key(const DhPrivKey* priv, DhPubKey* pub);

// Given a priv key and another party's pub key: calc shared secret.
// len of ss should obviously be R_LEN * LIMB_BYTE_COUNT.
void dh_calc_shared_secret(const DhPrivKey* priv, const DhPubKey* pub, u8* ss);


#endif // DH_H__
