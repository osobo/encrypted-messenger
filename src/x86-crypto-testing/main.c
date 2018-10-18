#include <stdio.h>
#include <string.h>
#include "spoch.h"


void print_bytes(const u8 a[], u32 len)
{
  u32 i;
  printf("{");
  for (i = 0; i < len; ++i) {
    if (i % 8 == 0) {
      printf("\n\t");
    }
    printf("%02x ", a[i]);
  }
  printf("\n}\n");
}


int main()
{
  const u32 dgst_len = 32;
  u8 dgst[dgst_len];
  u8 msg[] = {
    0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00
  };
  //const u32 msg_len = sizeof(msg)-1; // remove null term.
  const u32 msg_len = sizeof(msg);

  spoch(msg, msg_len, dgst_len, dgst);

  printf("the resuling dgst = ");
  print_bytes(dgst, dgst_len);

  u8 key[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
  };
  u8 tag[dgst_len];
  spoch_hmac(msg, msg_len, key, sizeof(key), dgst_len, tag);
  printf("MAC tag = ");
  print_bytes(tag, dgst_len);
  
  //fwrite(dgst, dgst_len, 1, stdout);
}
