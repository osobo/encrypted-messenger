#ifndef SPOCH_H__
#define SPOCH_H__

#include "machine_defs.h"


void spoch(const u8 msg[], u32 msg_len, u32 dgst_len, u8 dgst_out[]);

// Performs produces an hmac of the msg using spoch of digest length dgst_len.
// The size of mac_out is assumed to be the same as dgst_len.
void spoch_hmac(const u8 msg[], u32 msg_len, const u8 key[], u32 key_len,
                                              u32 dgst_len, u8 mac_out[]);


#endif // SPOCH_H__
