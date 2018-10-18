#include <stdio.h>
#include "chacha20.h"

void print_bytes(const u8 bytes[], u32 len)
{
  u32 i;
  printf("%u bytes:[", len);
  for (i = 0; i < len; ++i) {
    printf(" %02x", bytes[i]);
  }
  printf("]\n");
}


int main()
{
  ChaChaCtx ctx;

  // This key and nonce are taken from example / test 5 (see chacha20.c)
  const u8 key[] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
  };
  const u8 nonce[] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07
  };

  chacha_init(&ctx, key, nonce);

  /*
  // You can generate any multiple of 64 bytes of the keystream.
  // Remember: chacha_keystream() does _not_ reset ctr.
  // Comparing the printed keystream to the desired result from
  // https://tools.ietf.org/html/draft-agl-tls-chacha20poly1305-04#section-7
  // you'll see that it's correct.
  u8 keystream[512];
  chacha20_keystream(&ctx, sizeof(keystream), keystream);
  chacha_set_ctr(&ctx, 0, 0);
  print_bytes(keystream, sizeof(keystream));
  */

  u8 plain[] = "A super secret and null terminated string.";
  const u32 len = sizeof(plain); // includes null term.
  u8 cipher[len]; // Same size, ie the null term is also encrypted.
  u8 decipher[len];

  // If you only use chacha20_enc you do not need to reset the ctr.
  // So the simple use case is: init ctx with chacha_init() and then
  // call chacha20_enc any number of times to encrypt or decrypt data.

  // Encrypt plain text and store in cipher.
  chacha20_enc(&ctx, plain, len, cipher);

  // Ecrypt and decryfpt is the same op in chacha.
  // Decrypt cipher text and store in decpher.
  chacha20_enc(&ctx, cipher, len, decipher);

  printf("plain as hex data\n");
  print_bytes(plain, len);

  printf("\ncipher as hex data\n");
  print_bytes(cipher, len);

  printf("\ndecipher as hex data\n");
  print_bytes(decipher, len);

  printf("\nplain in ascii =\n{%s}\ndecipher in ascii =\n{%s}\n", plain, decipher);
}

