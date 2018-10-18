#include "machine_defs.h"

u32 getbtns();

void show_inc_message(char* str);


void wheel_tick(unsigned int wheel_val);


// Is called when a switch is flipped OFF.
// Takes number of switch as arg.
void sw_handler(uint8_t sw);

void time_tick();

void set_display(char* s0, char* s1, char* s2, char* s3);

void crash(char* s0, char* s1, char* s2, char* s3);

void init_ui();
