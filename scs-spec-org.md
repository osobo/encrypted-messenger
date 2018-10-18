
# Spec for  SCS-protocol
__(Secured Communication Session)__


In this text, let's assume peer A (or simply `A`) is connecting to peer B (or simply `B`). In other words, `B` must have been listening on some port `P` and `A` must've initiated a TCP-connection to `B` on port `P`.

### Initial contact
After establishing a connection over TCP peers `A` and `B` now have to initiate the _SCS_ through a handshake. Since `A` initiated the connection it will be `A` who begins the handshake. `A` does so by simply sending the raw _ASCII data_ `SCS INIT` followed by a null (or zero) byte. So `A` sends 9 bytes to start the handshake. In hexadecimal the 9 bytes sent would look like this: `0x53 0x43 0x53 0x20 0x49 0x4e 0x49 0x54 0x00`

### Key exchange
In response `B` will send the raw _ASCII data_ `SCS SKEY` (_SCS server key_), followed by a null byte and then the 512-bit _EC public key_ of `B` (__TODO 1__). So, in total `B` sends 72 bytes (9 bytes for the initial _ASCII data_ and null byte, 64 bytes for the key) (__TODO 1__). In hexadecimal it would look like this when `[KEY]` is replaced with the 64 (__TODO 1__) bytes of the public key of `B`: `0x53 0x43 0x53 0x20 0x53 0x4b 0x45 0x59 0x00 [KEY]`

After `A` has received the public key of `B`, `A` sends its public key in the same way but with different leading _ASCII data_. A starts by sending `SCS CKEY` (_SCS client key_) instead of `SCS SKEY` (_SCS server key_). In hexadecimal it would look like this when `[KEY]` is replaced with the 64 (__TODO 1__) bytes of the public key of `A`: `0x53 0x43 0x53 0x20 0x43 0x4b 0x45 0x59 0x00 [KEY]`.

### Shared key
Now that both peers have each other's public key they can both use _EC-cryptography_ to find the same _Shared secret_. In other words, they perform _Elliptic Curve Point Multiplication_ with their own _secret key_ (scalar) and the other peers _public key_ (point).

Let _n<sub>A</sub>_ and _n<sub>B</sub>_ be the secret keys of the two peers and _Q<sub>A</sub>_ and _Q<sub>B</sub>_ the public keys of the two peers. `A` then performs the calculation _n<sub>A</sub>Q<sub>B</sub>_ and `B` performs the calculation _n<sub>B</sub>Q<sub>A</sub>_. Both of these calculations will result in the same point (ie 64 bytes) which is the _shared secret_ (or `SS`).

The _shared key_ (or `SK`) has to be 32 bytes (256 bits) because of the encryption algorithm used (__TODO 3__). `SK` is derived from the 64 byte `SS` by _xor-ing_ the 32 most significant bytes of `SS` onto the 32 least significant bytes of `SS` (__TODO 4__). So, if `SS` is represented as an array of bytes called `ss_bytes` and `SK` as an array of bytes called `sk_bytes` the following _C code_ demonstrates how `SK` is derived from `SS`:
```c
assert(sizeof(sk_bytes) == 32);
assert(sizeof(ss_bytes) == 64);
unsigned int i;
for(i=0 ; i < 32 ; ++i) {
  sk_bytes[i] = ss_bytes[i] ^ ss_bytes[i+32];
}
```

### Key verification
The peers could easily verify that they have derived the same `SK` by using some _hash function_ and sending each other the hash of the key. Since we want to limit ourselves to as few cryptographic primitives as possible we don't want to do this. For now, let's just not perform any key verification? (__TODO 5__).

### Data transfer
Now that both `A` and `B` have securely established a _shared key_ they can securely transfer data between each other. Data is transferred in _envelopes_. An _envelope_ can contain from 1 byte up to 65535 (256<sup>2</sup>-1) bytes of data. If a peer has more than 65535 bytes of data to send it will have to be split up into multiple _envelopes_.

Let `D` be the sequence of bytes to send in an _envelope_ and let the length of `D` (in bytes) be `L` (so 1 <= `L` <= 65535). First the peer must generate a random nonce (let's call it `N`) for the _envelope_. _ChaCha20_ uses an 8 byte nonce as part of it's input, so the random nonce should be 8 bytes (__TODO 3 & 6__). The peer must then use _ChaCha20_ (__TODO 3__) to encrypt `D` with `N` and `SK`. Let's call the resulting _cipher text_ `C`. The length of `C` will also be `L`. The _envelope_ is then constructed by concatenating the following:

+ The _ASCII data_ `SCS SENV` (_SCS_ start envelope). In hexadecimal: `0x53 0x43 0x53 0x20 0x53 0x45 0x4e 0x56`.
+ A null byte. In hexadecimal: `0x00`.
+ The 8 byte nonce (`N`).
+ The length of the data (`L`) represented as a 2 byte unsigned integer. Eg if the size of the data sent is 1000 bytes then the bytes sent here would like this in hexadecimal: `0x03 0xE8`. Or if the size is 65535 (the max size) it would look like this `0xFF 0xFF`.
+ The cipher text (`C`) (`L` bytes).


## TODO
### 0 - Name
Should it be called _SCS protocol_?


### 1 - ECDH
Is ECDH going to be used for key exchange? What key size? Is it correct that the public key size is twice the private key size?

### 2 - EC curve parameters
Decide on what curve to use and specify it in the document above.

### 3 - ChaCha20
Is ChaCha20 (256-bit key) going to be used as the encryption algorithm?

### 4 - Secret to key
Should the two 64 halves of the byte _shared secret_ simply be _xor-ed_ to get the _shared key_? This way no hash function has to be implemented.

### 5 - Key verification
Should there be no verification of the _shared key_ before it is used?

### 6 - Envelope nonce
Should the envelope nonce simply be the 8 byte nonce to the _ChaCha20_ algorithm? It could also be 8+keysize (8+32 = 4) bytes to specify both _ChaCha20_ nonce and a keysized pad to _xor_ the _shared key_ with before encrypting. This is probably unnecessary, probably fine as is.

### 7 - Ending envelopes
Should envelopes be explicitly ended? Something like `SCS EENV` followed by the nonce?

### 8 - Version number
Should there be a version number in the initial `SCS INIT` message?

### 9 - Add example handshake and data transfer
