#include <stdio.h>
#include "dh.h"
#include "bigu.h"

void print_bytes(u8* bytes, u32 len)
{
  u32 i;
  printf("[%u bytes:");
  for (i = 0; i < len; ++i) {
    printf(" 0x%02x", bytes[i]);
  }
  printf("]\n");
}

int main()
{
  // A generates random private key.
  DhPrivKey priv_a;
  dh_random_priv_key(&priv_a);

  // So does b.
  DhPrivKey priv_b;
  dh_random_priv_key(&priv_b);

  // A calculates their public key.
  DhPubKey pub_a;
  dh_calc_pub_key(&priv_a, &pub_a);

  // So does B.
  DhPubKey pub_b;
  dh_calc_pub_key(&priv_b, &pub_b);

  // A and B exchange public keys (over network).
  // A: dh_pub_key_to_bytes(pub_a, pub_a_bytes);
  // B: dh_pub_key_to_bytes(pub_b, pub_b_bytes);
  // ...
  // A: dh_bytes_to_pub_key(pub_b_bytes, pub_b);
  // B: dh_bytes_to_pub_key(pub_a_bytes, pub_a);

  // A calculates shared secret using priv_a and pub_b.
  u8 ss_a[R_LEN*LIMB_BYTE_COUNT];
  dh_calc_shared_secret(&priv_a, &pub_b, ss_a);

  // B calculates shared secret using priv_b and pub_a.
  u8 ss_b[R_LEN*LIMB_BYTE_COUNT];
  dh_calc_shared_secret(&priv_b, &pub_a, ss_b);

  // Both A and B got the same shared secret:
  printf("A got ss =\n");
  print_bytes(ss_a, sizeof(ss_a));
  printf("B got ss =\n");
  print_bytes(ss_b, sizeof(ss_a));
}



