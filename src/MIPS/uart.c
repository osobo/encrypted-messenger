
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */
#include "machine_defs.h"
#include "uart.h"
#include "ui.h"


void uart_write_bytes(u32 len, const u8 bytes[len])
{
  u32 i;
  for (i = 0; i < len; ++i) {
    while (U1STA & (1 << 9)); // Wait for TX buf to be not full
    U1TXREG = bytes[i];
  }
}

void uart_read_bytes(u32 len, u8 buf[len])
{
  u32 i;
  u8 x;
  for (i = 0; i < len;) {
    if (U1STA & (1 << 1)) {
      crash("UART RX buffer", "overflowed", 0, 0);
    }
    if (U1STA & (1 << 0)) {
      // If we have incoming data to read
      x = U1RXREG;
      buf[i++] = x;
    }
  }
}

bool uart_has_rec()
{
  return U1STA & (1 << 0);
}


// Taken from example "hello-serial"
static int calculate_baudrate_divider(int sysclk, int baudrate, int highspeed)
{
	int pbclk, uxbrg, divmult;
	unsigned int pbdiv;

	divmult = (highspeed) ? 4 : 16;
	/* Periphial Bus Clock is divided by PBDIV in OSCCON */
	pbdiv = (OSCCON & 0x180000) >> 19;
	pbclk = sysclk >> pbdiv;

	/* Multiply by two, this way we can round the divider up if needed */
	uxbrg = ((pbclk * 2) / (divmult * baudrate)) - 2;
	/* We'll get closer if we round up */
	if (uxbrg & 1)
		uxbrg >>= 1, uxbrg++;
	else
		uxbrg >>= 1;
	return uxbrg;
}


void init_uart()
{
  // According to example "hello-serial"
	// Configure UART1 for 115200 baud, no interrupts
	//U1BRG = calculate_baudrate_divider(80000000, 115200, 0);
	U1BRG = calculate_baudrate_divider(80000000, 11520, 0);
	U1STA = 0;
	// 8-bit data, no parity, 1 stop bit
	U1MODE = 0x8000;
	// Enable transmit and recieve
	U1STASET = 0x1400;
  // Enable only read:
  //U1STASET = 1 << 12;

  // No interrupts!
  /*
  // Enable interrupts for uart read
  //IECSET(1) = 1 << 9; // U2RXIE
  IECSET(0) = 1 << 27; // U1RXIE
  IPCSET(6) = 0b11111;
  // UR1ISEL = 00: trigger on single read byte
  U1STASET = (1 << 6) | (1 << 7);
  */
  

}

