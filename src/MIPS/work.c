#include <pic32mx.h>
#include "mipslab.h"
#include "machine_defs.h"
#include "uart.h"
#include "chacha20.h"
#include "rng.h"
#include "dh.h"
#include "ui.h"
#include "spoch.h"


//#define DEBUG


#ifdef DEBUG

//#define PLAIN

void minisleep(u32 k)
{
  while (k-- > 0);
}


// Grabs the least sig 4 bits of passed byte and returns hex code.
char nibble_to_hex(u8 nibble)
{
  nibble &= 0x0F;
  if (nibble < 10) {
    return '0' + nibble;
  }

  return 'A' + (nibble-10);
}

// Just used for debugging
// Prints up to the first 8 bytes of the passed array bytes to the first line.
void print_buffer_start(u8 line, u8 bytes[], u8 len)
{
  len = (len == 0) || (len > 8) ? 8 : len;
  char str[17]; // 8 bytes = 16 hex digits + null term.
  str[16] = 0;

  u8 i;
  char c;
  for (i = 0; i < len; ++i) {
    c = nibble_to_hex(bytes[i] >> 4);
    str[i*2] = c;
    c = nibble_to_hex(bytes[i]);
    str[i*2 + 1] = c;
  }

  display_string(line, str);
}
#endif // DEBUG




static DhPrivKey priv_key;
static DhPubKey pub_key;
static u8 init_nonce[32];
static u8 env_nonce_ctr[8];

static u8 cipher_key[32];
static u8 peer_cipher_key[32];


const char env_header[] = "SCS SENV";

// Treats env_nonce_ctr as an 8 byte little endian int and increases it by one.
void increase_nonce_ctr()
{
  u8 carry = 1;
  u32 i;
  for (i = 0; carry > 0 && i < 8; ++i) {
    env_nonce_ctr[i] += carry;
    if (env_nonce_ctr[i] < carry) {
      // Overflow, we continue
      carry = 1;
    }
    else {
      carry = 0;
    }
  }
}

// Sends the passed string to the connected peer.
void send_message(const char* msg)
{
  u32 i, slen = 0;
  // Calculate string length
  for (i = 0; msg[i] != '\0'; ++i, ++slen);
  // The length of the body of the env will be slen+1 (we also send the null term).
  // The len in an env is a big endain 2 byte int:
  u8 len[2];
  len[0] = (slen+1) >> 8;
  len[1] = (slen+1) >> 0;


  // First send the header
  uart_write_bytes(sizeof(env_header), (u8*) env_header);
  // Then the 8 byte nonce
  uart_write_bytes(8, env_nonce_ctr);
  // Then the 2 bytes of msg len
  uart_write_bytes(2, len);
  // And then the message

#ifndef PLAIN
  // This is "normal" mode where we encrypt the message
  ChaChaCtx chacha;

  chacha_init(&chacha, cipher_key, env_nonce_ctr);
  //chacha_init(&chacha, hard_key, env_nonce_ctr);
  //chacha_init(&chacha, hard_key, hard_nonce);

  u8 ciph_buf[slen+1];

  chacha20_enc(&chacha, (u8*) msg, slen+1, ciph_buf);

  // Send the body of the env (cipher text)
  uart_write_bytes(slen+1, ciph_buf);

#else
  // In this mode we just send the string as the body of the env.
  // (For early debug)
  // Send message as null term string in envelope
  uart_write_bytes(slen+1, msg);
#endif

  // After using the current env nonce we must increase it so that the same
  // nonce is not used again.
  increase_nonce_ctr();
}



// Returns true if the data a and b point to is the same.
bool mem_eq(u32 len, const u8 a[len], const u8 b[len])
{
  while (len-- > 0) {
    if ((*a) != (*b)) {
      return FALSE;
    }
  }

  return TRUE;
}


// Assumes global vars have been set with our init_nonce and pub key
// Calculates out and peer's cipher keys (according to scs)
void calc_keys(DhPubKey* peer_pub_key, u8 peer_init_nonce[32])
{
  set_display("Calculating", "shared secret", "", "");

  u8 shared_secret[256];
  dh_calc_shared_secret(&priv_key, peer_pub_key, shared_secret);

  set_display("Calculating", "cipher keys", "", "");

  u8 buf[32 + 32 + 256];

  // First calc cipher_key
  memcpy(buf, init_nonce, 32);
  memcpy(buf+32, peer_init_nonce, 32);
  memcpy(buf+32+32, shared_secret, 256);
  spoch(buf, sizeof(buf), 32, cipher_key);

  // Then calc peer_cipher_key
  memcpy(buf, peer_init_nonce, 32);
  memcpy(buf+32, init_nonce, 32);
  memcpy(buf+32+32, shared_secret, 256);
  spoch(buf, sizeof(buf), 32, peer_cipher_key);
}

static const char init_msg[] = "SCS INIT";
static const u32 init_len = sizeof(init_msg);

static const char key_hdr_b[] = "SCS SKEY";
static const char nonce_hdr_b[] = "SCS SNON";
static const char key_hdr_a[] = "SCS CKEY";
static const char nonce_hdr_a[] = "SCS CNON";

static const u32 key_hdr_len = sizeof(nonce_hdr_a);
static const u32 nonce_hdr_len = sizeof(nonce_hdr_a);

// The exchange of pub keys and init nonces from peer A pov.
void init_scs_exchange_a(u8 peer_pub_buf[256], u8 peer_nonce[32])
{
  u8 tmp_buf[256];

  set_display("Exchanging", "keys and nonces", "", "");

  // Send init contact message
  uart_write_bytes(init_len, (u8*) init_msg);

  // Read and check header of key response
  uart_read_bytes(key_hdr_len, tmp_buf);
  if (!mem_eq(key_hdr_len, (u8*) key_hdr_b, tmp_buf)) {
    crash("Bad header in", "key response", 0, 0);
  }

  // Read public key (256 bytes or 2048 bits)
  uart_read_bytes(256, peer_pub_buf);

  // Read and check header of nonce response
  uart_read_bytes(nonce_hdr_len, tmp_buf);
  if (!mem_eq(nonce_hdr_len, (u8*) nonce_hdr_b, tmp_buf)) {
    crash("Bad header in", "nonce response", 0, 0);
  }

  // Read peer's init nonce
  uart_read_bytes(32, peer_nonce);

  // Now it's time for A to send CKEY and CNON

  // Send key header
  uart_write_bytes(key_hdr_len, (u8*) key_hdr_a);

  // Send the public key
  dh_pub_key_to_bytes(&pub_key, tmp_buf);
  uart_write_bytes(256, tmp_buf);

  // Send nonce header
  uart_write_bytes(nonce_hdr_len, (u8*) nonce_hdr_a);

  // Send the nonce
  uart_write_bytes(32, init_nonce);
}

// The exchange of pub keys and init nonces from peer B pov.
void init_scs_exchange_b(u8 peer_pub_buf[256], u8 peer_nonce[32])
{
  u8 tmp_buf[256];

  // Read init contact message
  uart_read_bytes(init_len, tmp_buf);
  if (!mem_eq(init_len, (u8*) init_msg, tmp_buf)) {
    crash("Bad init", "init msg", "from peer", 0);
  }

  set_display("Exchanging", "keys and nonces", "", "");

  // Send key header
  uart_write_bytes(key_hdr_len, (u8*) key_hdr_b);

  // Send the public key
  dh_pub_key_to_bytes(&pub_key, tmp_buf);
  uart_write_bytes(256, tmp_buf);

  // Send nonce header
  uart_write_bytes(nonce_hdr_len, (u8*) nonce_hdr_b);

  // Send the nonce
  uart_write_bytes(32, init_nonce);

  // Now it's time to read key and nonce from A:

  // Read and check header of key response
  uart_read_bytes(key_hdr_len, tmp_buf);
  if (!mem_eq(key_hdr_len, (u8*) key_hdr_a, tmp_buf)) {
    crash("Bad header in", "key response", 0, 0);
  }

  // Read public key (256 bytes or 2048 bits)
  uart_read_bytes(256, peer_pub_buf);

  // Read and check header of nonce response
  uart_read_bytes(nonce_hdr_len, tmp_buf);
  if (!mem_eq(nonce_hdr_len, (u8*) nonce_hdr_a, tmp_buf)) {
    crash("Bad header in", "nonce response", 0, 0);
  }

  // Read peer's init nonce
  uart_read_bytes(32, peer_nonce);
}

// The initial steps of a scs. Once this function returns the global vars
// cipher_key and peer_cipher_key will be set.
void init_scs()
{
  u8 peer_nonce[32];
  u8 pub_buf[256];
  DhPubKey peer_pub_key;

  set_display("Press any button", "to make", "connection", "");

  // Wait for either:
  //    + incomming connection, making us peer B
  //    + user input asking to init connection, making us peer A
  for (;;) {
    if (uart_has_rec()) {
      // Other peer has init connection: we are peer B
      init_scs_exchange_b(pub_buf, peer_nonce);
      break;
    }

    if (getbtns()) {
      // User has pressed a button to init connection: we are peer A
      init_scs_exchange_a(pub_buf, peer_nonce);
      break;
    }

  }

  dh_bytes_to_pub_key(pub_buf, &peer_pub_key);

  // We now have the peer's pub and init nonce: calc cipher_keys.
  calc_keys(&peer_pub_key, peer_nonce);
}


// Interrupt Service Routine
void user_isr()
{
  // Timer 2 interrupt (T2IF)
  if (IFS(0) & 0x00000100) {
    // Set T2IF to 0.
    IFS(0) &= 0xFFFFFEFF;
    time_tick();
  }

  // Wheel interrupt (AD1IF)
  if (IFS(1) & (1 << 1)) {
    wheel_tick(ADC1BUF0);

    IFS(1) &= ~(1 << 1);
    // Set done to 0.
    AD1CON1 &= ~1;
    // Set sample to 1.
    AD1CON1 |= 2;
  }

}


// Just init LEDs, btns, switches, uart and rng.
void first_init()
{
  // The addresses to TRISE and PORTE according to the task.
  volatile unsigned* tris_e = (volatile unsigned*) 0xbf886100;

  // Set the least sig byte to zero (ie output), leave the rest as is.
  *tris_e &= 0xFFFF00;

  // Set bits 5-11 to 1 (ie input). BTNn4-2 and all 4 switches
  TRISD |= 0x00000FE0;

  // Set bit 1 to 1 (ie input). BTN1
  TRISFSET = 0b0010;

  // Init the timer to 0.
  TMR2 = 0;
  // Set bit 15 to one to enable the timer.
  // Also set all three TCKPS bits to one (bits 4-6).
  T2CON |= 0x8070;
  // Set the period;
  PR2 = 31250;
  //PR2 = 3125; // 10th of a sec
  // Set the 8th bit of IFS0 (T2IF) to 0, ie reset the timeout flag.
  IFS(0) &= 0xFFFFFEFF;

  // Enable 8th bit if IEC0 (T2IE).
  IEC(0) |= 0x00000100;
  // Enable all three T2IP (bits 2-4) and all two T2IS (bits 0-1)
  // for MAXIMUM prio and sub prio.
  IPC(2) |= 0x0000001F;

  init_uart();
  init_rng();
}

// Probably implemented in assembly
extern void enable_interrupt();

// Init wheel, and interrups
void second_init()
{
  // Init analog wheel thing:

  /* PORTB.2 is analog pin with potentiometer*/
	AD1PCFG = ~(1 << 2);
	TRISBSET = (1 << 2);
	/* Use pin 2 for positive */
	AD1CHS = (0x2 << 16);

	/* Data format in uint32, 0 - 1024
	Manual sampling, auto conversion when sampling is done
	FORM = 0x4; SSRC = 0x7; CLRASAM = 0x0; ASAM = 0x0; */
	AD1CON1 = (0x4 << 8) | (0x7 << 5);

	AD1CON2 = 0x0;
	AD1CON3 |= (0x1 << 15);

	/* Turn on ADC */
	AD1CON1 |= (0x1 << 15);

  // Set AD1IF to 0.
  IFS(1) &= 2;
  // Enable bit 1 of IEC1 (AD1IE).
  IEC(1) |= 1 << 1;
  // Enable all three AD1IP (bits 26-28) and all two AD1IS (bits 24-25)
  // for MAXIMUM prio and sub prio.
  //IPC(6) |= 0x1F000000;
  IPC(6) |= 0x0F000000;

  // Set done to 0.
  AD1CON1 &= ~1;
  // Set sample to 1.
  AD1CON1 |= 2;

  // Enable interrupts globally.
  enable_interrupt();

  PORTE = 0;
}

// Returns true if buf contains a valid env header (SCS SENV).
bool is_env_header(u8 buf[])
{
  return mem_eq(sizeof(env_header), (u8*) env_header, buf);
}

// Waits for incomming "proper" (as in with an encrypted body) env and shows
// the user the message.
void process_proper_env()
{
  u8 buf[20];
  u8 env_nonce[8];

  // Read and check envelope header
  uart_read_bytes(sizeof(env_header), buf);
  if (!is_env_header(buf)) {
    crash("Bad envelope hdr", 0, 0, 0);
  }

  // Read nonce
  uart_read_bytes(8, env_nonce);

  // Read msg len:
  u16 len;
  uart_read_bytes(sizeof(len), buf);
  len = buf[0];
  len <<= 8;
  len |= buf[1];

  // Now read cipher text (body of env)
  uart_read_bytes(len, buf);

  // Msg should be null term
  char plain[len];
  ChaChaCtx chacha;
  chacha_init(&chacha, peer_cipher_key, env_nonce);
  //chacha_init(&chacha, hard_key, env_nonce);
  //chacha_init(&chacha, hard_key, hard_nonce);
  chacha20_enc(&chacha, buf, len, (u8*) plain);

  show_inc_message(plain);
}

#ifdef DEBUG
// Really just for debugging
void process_str_env()
{
  u8 buf[1024];
  const u32 nonce_len = 8;

  // Read and check envelope header
  uart_read_bytes(sizeof(env_header), buf);
  if (!is_env_header(buf)) {
    crash("Bad envelope hdr", 0, 0, 0);
  }

  // Read nonce
  uart_read_bytes(nonce_len, buf);
  // nonce is now in buf, use it!
  
  // Read msg len:
  u16 len;
  uart_read_bytes(2, buf);
  len = buf[0];
  len <<= 8;
  len |= buf[1];

  // Now read msg
  uart_read_bytes(len, buf);

  // Msg should be null term:
  show_inc_message((char*) buf);
}
#endif // DEBUG

// Performs init random generation and calculations.
// Generates random private key, init nonce and starting env nonce counter.
// Derives public key from the random private key.
void boot_process()
{
  // First we need to generate random private key, nonce for key exchange
  // and first envelope nonce

  set_display("Booting...", "Generating", "nonces", "press some btns");
  rng_data(sizeof(env_nonce_ctr), env_nonce_ctr);
  rng_data(sizeof(init_nonce), init_nonce);

  set_display("Booting", "Generating", "private key", "press some btns");
  dh_random_priv_key(&priv_key);

  // Next we calculate the public key
  set_display("Calculating", "public key", "", "");
  dh_calc_pub_key(&priv_key, &pub_key);
}

// The "main function"
void work()
{
#ifndef PLAIN
  boot_process();

  init_scs();
#endif

  second_init();

  init_ui();

  // After we init all we need to do in "main program" is process incomming
  // messages. Outgoing will be handled by interrupts.
  for (;;) {
#ifdef PLAIN
    process_str_env();
#else
    process_proper_env();
#endif
  }


}


