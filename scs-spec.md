
# Spec for  SCS-protocol
__(Secured Communication Session)__


In this text, let's assume peer A (or simply `A`) is connecting to peer B (or simply `B`). ~~In other words, `B` must have been listening on some port `P` and `A` must've initiated a TCP-connection to `B` on port `P`~~. In other wirds, `B` must be waiting for an incomming connection from `A` and `A` must initiate the connection.

### Initial contact
After establishing a connection over ~~TCP~~ peers `A` and `B` now have to initiate the _SCS_ through a handshake. Since `A` initiated the connection it will be `A` who begins the handshake. `A` does so by simply sending the raw _ASCII data_ `SCS INIT` followed by a null (or zero) byte. So `A` sends 9 bytes to start the handshake. In hexadecimal the 9 bytes sent would look like this: `0x53 0x43 0x53 0x20 0x49 0x4e 0x49 0x54 0x00`

### Key exchange
In response `B` will send the raw _ASCII data_ `SCS SKEY` (_SCS server key_), followed by a null byte and then the 2048-bit _DH public key_ of `B` (in big endian). So, in total `B` sends 265 bytes (9 bytes for the initial _ASCII data_ and null byte, 256 bytes for the key). Next `B` also needs to send their 256-bit nonce by first sending the _ASCII data_ `SCS SNON`, followed by a null byte and then the 256-bit nonce. In hexadecimal it would look like this when `[KEY]` is replaced with the 256 bytes of the public key of `B` and `[NONCE]` is replaced by the nonce of `B`:
```
0x53 0x43 0x53 0x20 0x53 0x4b 0x45 0x59 0x00 [KEY] 0x53 0x43 0x53 0x20 0x53 0x4e 0x4f 0x4e 0x00 [NONCE]
```

After `A` has received the public key and nonce of `B`, `A` sends its public key (in big endian) and nonce in the same way but with different leading _ASCII data_. A starts by sending `SCS CKEY` (_SCS client key_) instead of `SCS SKEY` (_SCS server key_) and also used `SCS CNON` instead of `SCS SCNON`. In hexadecimal it would look like this when `[KEY]` is replaced with the 256) bytes of the public key of `A`:
```
0x53 0x43 0x53 0x20 0x43 0x4b 0x45 0x59 0x00 [KEY] 0x53 0x43 0x53 0x20 0x43 0x4e 0x4f 0x4e 0x00 [NONCE].
```


### Shared keys
Now that both peers have each other's public key they can both use _Modular Exponatiation_ (see Diffie Hellman key exhange) to derive the same _Shared Secret_. In other words, if _x_ is the peer's private (256-bit) private key and _y_ is the other peer's public (2048-bit) public key, the peer calculates _x_^_y_ (mod _p_) where _p_ is the prime specified by the diffie hellman parameters (see appendix). The independent calculations on both peer devices will result in the same result, the _shared secret_ (or `SS`). The size of `SS` is the same as the public keys, ie 256 bytes.

From the shared secret `SS`, nonce from `A` (`NA`) and nonce from `B` (`NB`) two shared keys will be generated, all 256 bits (32 bytes) since that is the key size of ChaCha20. Let `spoch(l, msg)` be the spoch digest of length `l` of message `msg`.
```
CKA = spoch(32, append(NA, NB, SS)
CKB = spoch(32, append(NB, NA, SS)
```

So the length of the data that is hashed is always 32 + 32 + 256 = 320 bytes.

+ `CKA` Is used as the ChaCha20 key when `A` sends a message. So `A` encrypts with it and `B` decrypts with it.
+ `CKB` Is used as the ChaCha20 key when `B` sends a message. So `B` encrypts with it and `A` decrypts with it.


### Data transfer
Now that both `A` and `B` have securely established _shared keys_ they can securely transfer data between each other. Data is transferred in _envelopes_. An _envelope_ can contain from 1 byte up to 65535 (256<sup>2</sup>-1) bytes of data. If a peer has more than 65535 bytes of data to send it will have to be split up into multiple _envelopes_.

Let `D` be the sequence of bytes to send in an _envelope_ and let the length of `D` (in bytes) be `L` (so 1 <= `L` <= 65535). First the peer must generate a unique nonce (let's call it `N`) for the _envelope_. _ChaCha20_ uses an 8 byte nonce as part of it's input, so the envelope nonce should be 8 bytes. The peer must then use _ChaCha20_ to encrypt `D` with `N` and `CKA` (for `A`) or `CKB` (for `B`). Let's call the resulting _cipher text_ `C`. The length of `C` will also be `L`. The _envelope_ is then constructed by concatenating the following:

+ The _ASCII data_ `SCS SENV` (_SCS_ start envelope). In hexadecimal: `0x53 0x43 0x53 0x20 0x53 0x45 0x4e 0x56`.
+ A null byte. In hexadecimal: `0x00`.
+ The 8 byte nonce (`N`).
+ The length of the data (`L`) represented as a 2 byte unsigned integer in big endian. Eg if the size of the data sent is 1000 bytes then the bytes sent here would like this in hexadecimal: `0x03 0xE8`. Or if the size is 65535 (the max size) it would look like this `0xFF 0xFF`.
+ The cipher text (`C`) (`L` bytes).

### Suggestion for envelope nonce genereration
It is important that a unique nonce is used for each envelope. To ensure this is the case a peer could generate a random 8 byte nonce once and then increase it by one between each usage. That is, you can treat the nonce as a 8 byte integer and increase it by one before using it for encryption and inserting in into the envelope. This ensures that the peer uses unique nonces for every envelope.


## DH parameters

Suggestion for Diffie Hellman parameters. These are taken from rfc5114, section 2.3. The generator and public keys are 2048-bits, the private keys (exponent) 256-bits.

#### The prime number `p` in hexadecimal:
```
87A8E61D B4B6663C FFBBD19C 65195999 8CEEF608 660DD0F2
5D2CEED4 435E3B00 E00DF8F1 D61957D4 FAF7DF45 61B2AA30
16C3D911 34096FAA 3BF4296D 830E9A7C 209E0C64 97517ABD
5A8A9D30 6BCF67ED 91F9E672 5B4758C0 22E0B1EF 4275BF7B
6C5BFC11 D45F9088 B941F54E B1E59BB8 BC39A0BF 12307F5C
4FDB70C5 81B23F76 B63ACAE1 CAA6B790 2D525267 35488A0E
F13C6D9A 51BFA4AB 3AD83477 96524D8E F6A167B5 A41825D9
67E144E5 14056425 1CCACB83 E6B486F6 B3CA3F79 71506026
C0B857F6 89962856 DED4010A BD0BE621 C3A3960A 54E710C3
75F26375 D7014103 A4B54330 C198AF12 6116D227 6E11715F
693877FA D7EF09CA DB094AE9 1E1A1597
```

#### The generator `g` in hexadecimal:
```
3FB32C9B 73134D0B 2E775066 60EDBD48 4CA7B18F 21EF2054
07F4793A 1A0BA125 10DBC150 77BE463F FF4FED4A AC0BB555
BE3A6C1B 0C6B47B1 BC3773BF 7E8C6F62 901228F8 C28CBB18
A55AE313 41000A65 0196F931 C77A57F2 DDF463E5 E9EC144B
777DE62A AAB8A862 8AC376D2 82D6ED38 64E67982 428EBC83
1D14348F 6F2F9193 B5045AF2 767164E1 DFC967C1 FB3F2E55
A4BD1BFF E83B9C80 D052B985 D182EA0A DB2A3B73 13D3FE14
C8484B1E 052588B9 B7D2BBD2 DF016199 ECD06E15 57CD0915
B3353BBB 64E0EC37 7FD02837 0DF92B52 C7891428 CDC67EB6
184B523D 1DB246C3 2F630784 90F00EF8 D647D148 D4795451
5E2327CF EF98C582 664B4C0F 6CC41659
```

