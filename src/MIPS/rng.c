#include "machine_defs.h"
#include "spoch.h"
#include <pic32mx.h>
#include "ui.h"

void init_rng()
{
  // We just need to init TMR3

  // Init the timer to 0.
  TMR3 = 0;
  // Enable TMR3;
  T3CONSET = 1 << 15; // T3CON.ON
  // TMR3.TCKPS = 00
  T3CONCLR = (1 << 4) | (1 << 5);
  // Set the period;
  PR3 = 256;
}

static u8 rng_byte()
{
  u32 i;
  static const u32 p = 11113; // Some prime cause why not
  for (i = 0; i < p; ++i);
  u8 x = TMR3;
  return x;
}

// Waits a "random" amount of time depentent on user input on the buttons.
static void btn_wait()
{
  u32 k = 0;
  u8 x;

  // Wait until any button is pressed and store the button status in x.
  while (!(x = getbtns()));

  // Set k to a value unique to the button status in x.
  if (x & (1 << 0)) {
    k += 5;
  }
  if (x & (1 << 1)) {
    k += 23;
  }
  if (x & (1 << 2)) {
    k += 59;
  }
  if (x & (1 << 3)) {
    k += 79;
  }

  // Wait k iterations before exiting.
  while (k-- > 0);
}

void rng_data(u32 len, void* out_data)
{
  const u32 buf_len = 10*len;
  u8 buf[buf_len];
  u32 i;
  for (i = 0; i < buf_len; ++i) {
    //while (!getbtns());
    btn_wait(); // Wait some, depends on user input
    buf[i] = rng_byte();
  }

  spoch(buf, buf_len, len, (u8*) out_data);
}

