# Spec for SpoCh
***Sponge Chacha hash function***

It is a variable digest length (1 .. 2^32-1 bytes) hash function that works on input of arbitrary length.

## Clarifications

### _ChaCha20(.)_ and _ChaCha20M(.)_
Both `ChaCha20(Matrix)` and `ChaCha20M(Matrix)` perform 10 double rounds on their argument and adds the original input Matrix to the result efter the double rounds. `ChaCha20(Matrix)` returns an array of 64 bytes, this is the function used to generate the keystream when using ChaCha for encryption. `ChaCha20M(Matrix)` returns the same data but in form of a _ChaCha matrix_. Ie, `ChaCha20M(Matrix)` returns the final matrix instead of first serializing it to a sequence of bytes. `ChaCha20(Matrix)` can be defined in terms of `ChaCha20M(Matrix)` as follows:
```
ChaCha20(Matrix):
    ResultMatrix <- ChaCha20M(Matrix)
    return MatrixToBytes(ResultMatrix)
```

### Digest Length
The hash function can produce digests in length ranging from 1 byte up to (excluding) 2^32 bytes. This length will be refered to as `L`.

### Input message
The input (message) can be of any any number of bytes length (0 or more). The unaltered input message will be refered to as `Msg` and the length of it (in bytes) will be called `len(Msg)`.


## The specification
`SpoCh` is a sponge function. To specify the function we need only specify the following:

+ A state memory `S`
+ A transformation function `F : S -> S`
+ A pad function `P : Bytes -> Bytes`

`SpoCh` also specifies an initialization vector (`IV`) other than an all zero state which sponge functions usually use.

### The State Memory
`S` is a 4 by 4 matrix of 32-bit words. Ie a _"ChaCha matrix"_. The `I/O area` or `R` is words 14 and 15, ie the two bottom right words, ie the nonce words, of the matrix `S`. The rest of `S` is the so called _capacity_, or `C`. This means that 2 words (8 bytes) of message are processed at a time. The 8 bytes of message are interpreted as two words and are XORed onto the nonce words of the state. Let the array of 8 message bytes be called `M`, and let `V` and `W` be two 4 byte words where `V0` is the least significant byte of `V`, and so on.

+ `V0 = M[0]`
+ `V1 = M[1]`
+ `V2 = M[2]`
+ `V3 = M[3]`
+ `W0 = M[4]`
+ `W1 = M[5]`
+ `W2 = M[6]`
+ `W3 = M[7]`

Then `V` would be XORed onto the word with index 14 in the state and `W` would be XORed onto the word with index 15 in the state.

### The Transformation Function (`F`)
The transformation is simpy `ChaCha20M` (which keeps the output in the same format) applied twice to the state. So, after XORing the message block onto `R` the new state is calculated as follows.
```
F(S):
    TempState <- ChaCha20M(S)
    return ChaCha20M(TempState)
```

### Padding function
The padding in `SpoCh` works just like padding in _Keccak_. There is always at least one byte of padding appended to the message, regardless of its original length. There are two scenarios:

__Scenario 1:__

The length of the original message `len(Msg)` is one less than a multiple of 8, ie the length is congruent to 7 (or -1) mod 8. In this scenario a single byte is appended as padding and that byte is 0x81. So if `len(Msg) % 8 == 7`, then `P(Msg) = append(Msg, 0x81)`.

__Scenario 2:__

If `len(Msg) % 8 != 7`. Let `Msg' = append(Msg, 0x80)`. Next, keep appending 0x00 until the length is congruent to 7 in mod 8. Finally 0x01 is appended.

So:
```
P(Msg):
    if (len(Msg) % 8 == 7):
        return append(Msg, 0x81)
    else:
        Msg' <- append(Msg, 0x80)
        zeros_needed <- 7 - (len(Msg') % 8)
        repeat zeros_needed times:
            Msg' <- append(Msg', 0x00)
        return append(Msg', 0x01)
```

### Initialization vector
The `IV` will depend on `L`, the desired digest length. Since `L` will at most be 2^32-1, it can be expressed as a 32 bit word. Let `N` be `L` expressed a 32-bit unsigned integer. Next, let `P` be an array of 32 bytes:
```
P <- {
  0x02, 0x03, 0x05, 0x07,
  0x0B, 0x0D, 0x11, 0x13,
  0x17, 0x1D, 0x1F, 0x25,
  0x29, 0x2B, 0x2F, 0x35,
  0x3B, 0x3D, 0x43, 0x47,
  0x49, 0x4F, 0x53, 0x59,
  0x61, 0x65, 0x67, 0x6B,
  0x6D, 0x71, 0x7F, 0x83,
}
```
Next, construct a _ChaCha matrix_ `X` with the ordinary salsa constant, `P` as key  the counter set to `N` (so most sig ctr word is 0, least sig ctr word is `N`) and the nonce all zero. Run this state through the transformation functon `F` (ie through `ChaCha20M` twice). The result is the `IV`.

## Overview
A simple description of the whole algorithm:
```
Input: Msg (array of bytes), L (desired digest length as 32 bit word)
Output: Dgst (the resulting digest of L bytes)

PMsg <- P(Msg)
S <- IV(L)

# Let MsgBlock be set to the next 8 byte chunk of PMsg each iteration
for MsgBlock in PMsg:
    MsgNonce <- BytesToWords(MsgBlock)
    OldNonce <- ExtractNonce(S)
    NewNonce <- {
        MsgNonce[0] ^ OldNonce[0],
        MsgNonce[1] ^ OldNonce[1]
    }
    S <- SetNonce(NewNonce)
    F <- F(S)

Dgst <- {}

while len(Dgst) < L:
    N <- ExtractNonce(S)
    Dgst <- append(Dgst, N)
    S <- F(S)

# len(Dgst) is now multiple of 8, might be more than L. Truncate
# to ensure correct length.
Dgst <- Truncate(Dgst, L)
```

## Test vectors

The following have all used digest length (`L`) 32 bytes.

### Input message: empty string (`len(Msg) = 0`)
__32 byte spoch hash:__
```
d5 dd f7 5f 5f 36 d8 a0 
62 45 8c cc 5a 58 a0 a0 
30 80 8b 12 15 d0 85 4a 
84 58 47 03 27 33 24 26 
```

### Input message: `"hello"`
__32 byte spoch hash:__
```
2b 65 0e 81 de 2a 54 43
10 75 c2 6d 45 16 1a 95
66 92 3b 70 d9 c0 64 67
5a 7a 72 54 a1 4c c9 37
```

### Input message: `helln`
__32 byte spoch hash:__
```
88 4f e4 0a df a9 2f 2e
3b 3f 62 db 2f 29 92 3e
78 45 f1 84 51 34 c9 c1
df cc cd 48 a0 e6 49 1f
```

### Input message: `0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00`
__32 byte spoch hash:__
```
23 fd a9 7e 89 41 5a c9
df 84 33 39 6e cc f7 6b
84 d2 e1 65 5e a3 0b 1e
3e 24 b6 37 3d a3 bc 4a
```

### Input message: `0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x01`
__32 byte spoch hash:__
```
bc d6 b3 34 d9 c3 58 2c
1a c6 93 ca b1 fb 97 2f
c3 f3 b7 92 ea 4e bb 30
03 1c 7d eb 4c d2 36 70
```

### Input message: `0x00 0x00 0x01 0x00 0x00 0x00 0x00 0x00`
__32 byte spoch hash:__
```
e4 22 f7 25 ce 28 0c cc
e3 b9 2f bc 8b 89 86 f4
fe d3 c4 7b 0f e2 41 f9
7b a3 a3 f8 0d 25 bc 75
```


