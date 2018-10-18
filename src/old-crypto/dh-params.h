#ifndef DH_PARAMS_DH__
#define DH_PARAMS_DH__

#include "bigu.h"

// TODO: Include Limb defs and whatnot.
// Make p, p_prim and g of type Limb[].

/*
 * This is the 2048-bit MODP Group with 256-bit Prime Order Subgroup defined
 * in rfc5114 section 2.3.
 */

// They're 2048 bits (= 64 limbs = 256 bytes).
// So R = 2^2048, rlen = 64.

#define R_LEN 64

extern int p[];

// Not from the doc but calculated based on p (and R).
// Chosen so that p*p_prim = -1 (mod R).
extern int p_prim[];

//extern int g[];
extern int gm[];

extern const MontgomeryCtx mctx;


#endif // DH_PARAMS_DH__
