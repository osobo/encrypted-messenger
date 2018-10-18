#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "mipslab.h"  /* Declatations for these labs */
#include "machine_defs.h"
#include "uart.h"
#include "chacha20.h"
#include "rng.h"
#include "dh.h"


#define MSG_READ_ROW 1
#define MSG_WRITE_ROW 3


// Returns a byte where all bits are 0 except
// + bit 0: status of swtich 1
// + bit 2: status of swtich 2
// + bit 3: status of swtich 3
// + bit 4: status of swtich 4
u8 getsw()
{
  // Shift the contents of PORTE so that the desired four bits
  // are the four least sig bits and store in res.
  u8 res = PORTD >> 8;

  // Ensure that the rest of res is 0.
  res &= 0xF;

  return res;
}


// Returns a byte where all bits are 0 except
// + bit 0: status of BTN1
// + bit 2: status of BTN2
// + bit 3: status of BTN3
// + bit 4: status of BTN4
u8 getbtns()
{
  u8 d = (PORTD >> 4) & 0x0000000E;
  u8 f = (PORTF >> 1) & 0x00000001;
  return d | f;
}

// The column where the next char is to be written.
uint8_t write_col = 0;

// The current "write char", as in the char to show as current marker
// and to save to write_msg_buf when the user chooses to.
// Init to '\0' baceause why not
char write_char = '\0';

// This buffer holds the currently entered message as a null term string.
char write_msg_buf[17] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0,
};

// One of the character classes consists of a bunch of symbols, they are
// defined below.
static char symbols_array[] = ".,!?+-_*/:;\\(){}<>[]=^~|\"'";

// Range char classes and defined by a starting character and a "range", as in
// how many characters the class consists of. Eg, all upper case letters:
// start at 'A' and go on for 26 chars.
//
// Set char classes are defined by an array of chars, eg the symbols_array
// above.


typedef enum
{
  RANGE_CLASS,
  SET_CLASS,
} CharClassType;

typedef struct
{
  CharClassType type;
  union {
    struct {
      char start_char;
      uint8_t char_count;
    } range;
    struct {
      char* set_array;
      uint8_t char_count;
    } set;
  } u;
} CharClass;

static CharClass lower_case = {
  .type = RANGE_CLASS,
  .u = {
    .range = {
      .start_char = 'a',
      .char_count = 26,
    }
  },
};

static CharClass upper_case = {
  .type = RANGE_CLASS,
  .u = {
    .range = {
      .start_char = 'A',
      .char_count = 26,
    }
  }
};

static CharClass digits = {
  .type = RANGE_CLASS,
  .u = {
    .range = {
      .start_char = '0',
      .char_count = 10,
    }
  }
};

static CharClass symbols = {
  .type = SET_CLASS,
  .u = {
    .set = {
      .set_array = symbols_array,
      .char_count = sizeof(symbols_array)-1, // Exclude null term.
    }
  }
};

// Default to lower_case because why not.
static CharClass* current_char_class = &lower_case;


// Using the current char class, translate the wheel value (0--1023)
// to a char.
char wheel_val_to_char(u32 wheel_val)
{
  if (current_char_class->type == RANGE_CLASS) {
    u32 r = 1023 / (current_char_class->u.range.char_count) + 1;
    u32 offset = wheel_val / r;
    return (current_char_class->u.range.start_char) + offset;
  }
  else { // if type = SET_CLASS
    u32 r = 1023 / (current_char_class->u.set.char_count) + 1;
    u32 offset = wheel_val / r;
    return (current_char_class->u.set.set_array)[offset];
  }
}


// Displays the passed string as an incomming message on screen.
void show_inc_message(char* str)
{
  display_string(MSG_READ_ROW, str);
  display_update();
}

// Displays the current "write marker" on display. Ie, the current write char
// on current write column. Displays the char "inverted".
void display_write_marker()
{
  textbuffer[MSG_WRITE_ROW][write_col] = ~write_char;
}


// Called by interrupt when there's a new wheel_value.
// Updates the current write char and write marker according
// to the wheel value.
void wheel_tick(u32 wheel_val)
{
  char c = wheel_val_to_char(wheel_val);

  // Do nothing if the char did not update.
  if (c == write_char) {
    return;
  }

  write_char = c;

  display_write_marker();
  display_update();
}

// Called when switch status should be checked.
void handle_sw()
{
  u8 sw = getsw();

  // We only care about the state of the two least sig ones.
  sw &= 0b11;

  // The switches 
  switch (sw) {
    case 0:
      current_char_class = &lower_case;
      break;
    case 1:
      current_char_class = &upper_case;
      break;
    case 2:
      current_char_class = &digits;
      break;
    case 3:
      current_char_class = &symbols;
      break;
  }

}

void lock_write_char(char c)
{
  write_msg_buf[write_col] = c;
  if (write_col < 15) {
    ++write_col;
    display_string(MSG_WRITE_ROW, write_msg_buf);
    display_write_marker();
    display_update();
  }
}

void write_backspace()
{
  // Do nothing if at start.
  if (write_col == 0) {
    return;
  }

  --write_col;
  write_msg_buf[write_col] = 0;
  display_string(MSG_WRITE_ROW, write_msg_buf);
  display_write_marker();
  display_update();
}


void btn_handler(uint8_t btn)
{
  // We are now connected
  // Buttons are used for writing messages and sending:
  
  PORTE = 3;

  switch (btn) {
    // If BTN1: save current write char.
    case 1:
      lock_write_char(write_char);
      break;

    // if BTN2: write space (' ')
    case 2:
      lock_write_char(' ');
      break;

    // if BTN3: backspace
    case 3:
      write_backspace();
      break;

    case 4:
      send_message(write_msg_buf);
  }
}

void time_tick()
{
  // No interrupts for buttons, check if buttons are pressed each tick.
  u8 btns = getbtns();
  if (btns & (1 << 0))
    btn_handler(1);
  if (btns & (1 << 1))
    btn_handler(2);
  if (btns & (1 << 2))
    btn_handler(3);
  if (btns & (1 << 3))
    btn_handler(4);

  // Interrups only for sw falling edge. Handle switch each tick.
  handle_sw();
}


#define DISP_NOT_NULL(n, s)\
  do {\
    if (s) {\
      display_string(n, s);\
    }\
  } while (FALSE);

void set_display(char* s0, char* s1, char* s2, char* s3)
{
  DISP_NOT_NULL(0, s0);
  DISP_NOT_NULL(1, s1);
  DISP_NOT_NULL(2, s2);
  DISP_NOT_NULL(3, s3);
  display_update();
}

void crash(char* s0, char* s1, char* s2, char* s3)
{
  set_display(s0, s1, s2, s3);
  u8 leds = 1;
  u32 k;
  for (;;) {
    if (leds == 0) {
      leds = 1;
    }
    PORTE = leds;
    leds = leds << 1;
    for (k = 0; k < 40000; ++k);
  }
}

void init_ui()
{
  set_display("Read Message:", "", "Write Message:", "");
  display_write_marker();
  display_update();
}

