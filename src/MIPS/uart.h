#ifndef UART_H__
#define UART_H__

#include "machine_defs.h"


void init_uart();
void uart_write_bytes(u32 len, const u8 bytes[len]);
void uart_read_bytes(u32 len, u8 buf[len]);
// Returns true if we have received something over uart
// that we have not read / processed yet.
bool uart_has_rec();


#endif // UART_H__
