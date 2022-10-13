/*
 * Komi32 version 4.3 from komihash version 4.3
 * Copyright (c) 2022 David W. Gero
 * Copyright (c) 2021 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This is David Gero's 32-bit fork of Aleksey Vaneev's komihash.
 * It is portable, uses no special CPU instructions (unlike komihash,
 * which uses a multiply of two 64-bit numbers for a 128-bit result),
 * passes all the tests of SMHasher3, and is perfect for hash tables
 * with up to 4 billion buckets.
 * It is the fastest portable 32-bit hasher for byte strings of
 * length < 64 as tested against all the other portable 32-bit hashers
 * in SMHasher3.
 * For byte strings of length >= 64, Mult32 is faster.
 */

#ifndef KOMI32_H
#define KOMI32_H

#include <stdint.h>
#include <string.h>

#ifndef COMPILER_DEFS
#define COMPILER_DEFS 1

/* comment out the next line if you want the oppposite
 * endianness of the native processor
 */
#define NO_SWAP 1
/* comment out the next line if your processor requires aligned reads */
#define ALLOW_UNALIGNED_READS 1
/* comment out the next line if your compiler doesn't have __builtin_expect */
#define COMPILER_HAS_EXPECT 1
/* comment out the next line if your compiler doesn't have __builtin_prefetch */
#define COMPILER_HAS_PREFETCH 1

#if defined(COMPILER_HAS_EXPECT) && COMPILER_HAS_EXPECT
  #define likely(x)   __builtin_expect(!!(x), 1)
  #define unlikely(x) __builtin_expect(!!(x), 0)
#else
  #define likely(x)   (!!(x))
  #define unlikely(x) (!!(x))
#endif

#if defined(COMPILER_HAS_PREFETCH) && COMPILER_HAS_PREFETCH
  #define prefetch(x) __builtin_prefetch(x)
#else
  #define prefetch(x) do {(void)(x);} while (0)
#endif

/* make sure either LITTLE_ENDIAN or BIG_ENDIAN is defined,
 * but both are not defined.
 */
#if !defined(LITTLE_ENDIAN)
  #if !defined(BIG_ENDIAN)
    /* Neither LITTL_ENDIAN nor BIG_ENDIAN are defined */
    /* comment out the next line if your processor is big-endian */
    #define LITTLE_ENDIAN 1
    #if !defined(LITTLE_ENDIAN) /* previous line was commented out */
      #define BIG_ENDIAN 1
    #endif
  #endif
#else
  /* LITTLE_ENDIAN is defined */
  #if LITTLE_ENDIAN
    /* LITTLE_ENDIAN is defined and not zero */
    #if defined(BIG_ENDIAN)
      #undef BIG_ENDIAN
    #endif
  #else
    /* LITTLE_ENDIAN is defined, but is zero */
    #if defined(BIG_ENDIAN)
      #if BIG_ENDIAN
        /* BIG_ENDIAN is defined and not zero */
        #undef LITTL_ENDIAN
      #else
        /* BIG_ENDIAN is defined, but is zero */
        /* LITTLE_ENDIAN and BIG_ENDIAN are both zero */
        #error "LITTLE_ENDIAN and BIG_ENDIAN are both zero!"
      #endif /* BIG_ENDIAN not zero */
    #endif /* defined(BIG_ENDIAN) */
  #endif /* LITTLE_ENDIAN not zero */
#endif /* defined(LITTLE_ENDIAN) */

#if !defined(NO_SWAP) || !defined(LITTLE_ENDIAN)
  #if defined(_MSC_VER)
    #define BSWAP64(x) _byteswap_uint64(x)
  #elif (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
    #define BSWAP64(x) __builtin_bswap64(x)
  #elif defined(__has_builtin) && __has_builtin(__builtin_bswap64)
    #define BSWAP64(x) __builtin_bswap64(x)
  #elif defined(bswap64)
    #define BSWAP64(x) bswap64(x)
  #else
    #error "no 64-bit byte swap defined!"
    /* or you could use this:
     static inline uint64_t BSWAP64(uint64_t x) {
     uint32_t xhi = (uint32_t)(x >> 32);
     uint32_t xlo = (uint32_t)x;
 
     xhi = ((xhi & 0xFF000000) >> 24) |
           ((xhi & 0x00FF0000) >>  8) |
           ((xhi & 0x0000FF00) <<  8) |
           ((xhi & 0x000000FF) << 24);
     xlo = ((xlo & 0xFF000000) >> 24) |
           ((xlo & 0x00FF0000) >>  8) |
           ((xlo & 0x0000FF00) <<  8) |
           ((xlo & 0x000000FF) << 24);
     return ((uint64_t)xlo << 32) | (uint64_t)xhi;
     }
     */
  #endif /* BSWAP64 choices */
#endif /* !defined(NO_SWAP) */

#endif /* COMPILER_DEFS */

#if defined(ALLOW_UNLAIGNED_READS) && ALLOW_UNALIGNED_READS
  #if defined(NO_SWAP)
    #define GET_U32(m,i) (*(uint32_t*)((m)+(i)))
  #else
    #define GET_U32(m,i) BSWAP(*(uint32_t*)((m)+(i)))
  #endif
#else
  #if defined(NO_SWAP)
    static inline uint32_t GET_U32(const uint8_t* m, const uint32_t i) {
        uint32_t val;

        memcpy(&val, (m)+(i), 4);
        return val;
    }
  #else
    static inline uint32_t GET_U32(const uint8_t* m, const uint32_t i) {
        uint32_t val;
    
        memcpy(&val, (m)+(i), 4);
        return BSWAP(val);
    }
  #endif
#endif

/*------------------------------------------------------------*/

/*
 * Function builds an unsigned 32-bit value out of remaining bytes in a
 * message, and pads it with the "final byte". This function should only be
 * called if less than 4 bytes are left to read.
 *
 * @param Msg Message pointer, alignment is unimportant.
 * @param MsgLen Message's remaining length, in bytes; can be 0.
 * @param fb Final byte used for padding.
 */
/* Can be called with MsgLen == 0 to 3 */
static inline uint32_t komi32_final_bytes(const uint8_t * const Msg,
                                          const size_t MsgLen,
                                          uint32_t fb) {
    uint32_t m = 0;

    memcpy(&m, Msg, MsgLen);
    return m | (fb << (MsgLen << 3));
}

/*------------------------------------------------------------*/

/* Need to be able to multiply two 32-bit numbers and get a 64-bit result,
 * which will be split up into the lower 32 bits (rl) and upper 32 bits (rh).
 */
#define kh_m64(m1,m2,rl,rh) do { \
    uint64_t r = (uint64_t)(m1) * (uint64_t)(m2); \
\
    *rl = (uint32_t)r; \
    *rh = (uint32_t)(r >> 32); \
} while (0)

/* Common hashing round reading 8-byte input, using the "r1l" and "r1h"
 * temporary variables.
 */
#define KOMI32_HASH8(m)                        \
    kh_m64(Seed1 ^ GET_U32(m, 0),              \
           Seed5 ^ GET_U32(m, 4), &r1l, &r1h); \
    Seed5 += r1h;                              \
    Seed1 = Seed5 ^ r1l;

/* Common hashing round without input, using the "r1l" and "r1h" temporary
 * variables.
 *
 * The three instructions in the KOMI32_HASHROUND macro represent the
 * simplest constant-less PRNG, scalable to any even-sized state
 * variables, with Seed1 being the PRNG output (2^32 PRNG period).
 * It passes PractRand tests with rare non-systematic "unusual"
 * evaluations.
 *
 * To make this PRNG reliable, self-starting, and eliminate a risk of
 * stopping, the following variant can be used, which is a "register
 * checker-board", a source of raw entropy.
 *
 * Seed5 += r1h + 0xAAAAAAAA;
 *
 * (The 0xAAAA... constant should match Seed5's size; essentially,
 * it is a replication of the "10" bit-pair; it is not an arbitrary
 * constant.)
 */
#define KOMI32_HASHROUND()            \
    kh_m64(Seed1, Seed5, &r1l, &r1h); \
    Seed5 += r1h;                     \
    Seed1 = Seed5 ^ r1l;

/* Seed hashing round with 4-byte input, using the "r1l" and "r1h"
 * temporary variables.
 */
#ifndef KOMI32_SEEDHASH4
#define KOMI32_SEEDHASH4(x)                                   \
    kh_m64(Seed1 ^ ((x) & UINT32_C(0x55555555)),              \
           Seed5 ^ ((x) & UINT32_C(0xAAAAAAAA)), &r1l, &r1h); \
    Seed5 += r1h;                                             \
    Seed1 = Seed5 ^ r1l;
#endif

/* Handle reading final 0 to 7 bytes */
#define KOMI32_FINALIZE() do { \
    /* Msg[MsgLen - 1] is guaranteed to be valid */ \
    const uint32_t fb = 1 << (Msg[MsgLen - 1] >> 7); \
    const uint8_t numHash8 = (uint8_t)MsgLen; \
\
    /* This is slightly different from komihash, but still works */ \
    switch (numHash8) { \
        case 7: \
        case 6: \
        case 5: \
        case 4: \
            Seed1 ^= GET_U32(Msg, 0); \
            Seed5 ^= komi32_final_bytes(Msg + 4, MsgLen - 4, fb); \
            break; \
        case 3: \
        case 2: \
        case 1: \
        case 0: \
            Seed1 ^= komi32_final_bytes(Msg, MsgLen, fb); \
    } \
\
    KOMI32_HASHROUND(); \
    KOMI32_HASHROUND(); \
} while (0)

/*------------------------------------------------------------*/

/* KOMI32 hash function */

/*
 * @param in The message to produce a hash from. The alignment of this
 * pointer is unimportant.
 * @param len Message's length, in bytes.
 * @param UseSeed Optional value, to use instead of the default seed. To use
 * the default seed, set to 0. The UseSeed value can have any bit length and
 * statistical quality, and is used only as an additional entropy source. May
 * need endianness-correction if this value is shared between big- and
 * little-endian systems.
 */
static inline uint32_t Komi32_impl(const void * const in, const size_t len, uint64_t UseSeed) {
    /* Allow the compiler to put Msg in a register */
    const uint8_t* Msg = (const uint8_t*)in;
    /* Allow the compiler to put MsgLen in a register */
    uint64_t MsgLen = (uint64_t)len;
    uint32_t r1l, r1h, r2l, r2h;

    /* Initialize the pseudo-random number generator from the seed */
    uint32_t Seed1 = UINT32_C(0xC5A308D3);
    uint32_t Seed5 = UINT32_C(0xB8D01377);

    UseSeed ^= MsgLen;
    KOMI32_SEEDHASH4((uint32_t) UseSeed);
    KOMI32_SEEDHASH4((uint32_t)(UseSeed >> 32));

    if (unlikely(MsgLen == 0)) {
        KOMI32_HASHROUND();
        KOMI32_HASHROUND();

        return Seed1;
    }

    /* From here on, Msg[MsgLen - 1] is always valid */

    prefetch(Msg);

    if (likely(MsgLen >= 32)) {
        uint32_t Seed2 = UINT32_C(0x03707344) ^ Seed1;
        uint32_t Seed3 = UINT32_C(0x299F31D0) ^ Seed1;
        uint32_t Seed4 = UINT32_C(0xEC4E6C89) ^ Seed1;
        uint32_t Seed6 = UINT32_C(0x34E90C6C) ^ Seed5;
        uint32_t Seed7 = UINT32_C(0xC97C50DD) ^ Seed5;
        uint32_t Seed8 = UINT32_C(0xB5470917) ^ Seed5;
        uint32_t r3l, r3h, r4l, r4h;

        do {
            /* The "shifting" arrangement below does not increase
             * individual SeedN's PRNG period beyond 2^32, but reduces a
             * chance of any occassional synchronization between PRNG lanes
             * happening. Practically, Seed1-4 together become a single
             * fused 128-bit PRNG value, having a summary PRNG period of
             * 2^34.
             */

            kh_m64(Seed1 ^ GET_U32(Msg,  0),
                   Seed5 ^ GET_U32(Msg,  4), &r1l, &r1h);
            Seed5 += r1h;
            kh_m64(Seed2 ^ GET_U32(Msg,  8),
                   Seed6 ^ GET_U32(Msg, 12), &r2l, &r2h);
            Seed2  = Seed5 ^ r2l;
            Seed6 += r2h;
            kh_m64(Seed3 ^ GET_U32(Msg, 16),
                   Seed7 ^ GET_U32(Msg, 20), &r3l, &r3h);
            Seed3  = Seed6 ^ r3l;
            Seed7 += r3h;
            kh_m64(Seed4 ^ GET_U32(Msg, 24),
                   Seed8 ^ GET_U32(Msg, 28), &r4l, &r4h);
          
            Msg    += 32;
            prefetch(Msg);
          
            Seed4   = Seed7 ^ r4l;
            Seed8  += r4h;
            Seed1   = Seed8 ^ r1l;
            MsgLen -= 32;
        } while (likely(MsgLen >= 32));

        Seed5 ^= Seed6 ^ Seed7 ^ Seed8;
        Seed1 ^= Seed2 ^ Seed3 ^ Seed4;
    }

    /* MsgLen == 0 to 31 */

    {
        uint8_t numHash8 = (MsgLen >> 3) & 3;

        switch (numHash8) {
            case 3: KOMI32_HASH8(Msg); Msg += 8;
            case 2: KOMI32_HASH8(Msg); Msg += 8;
            case 1: KOMI32_HASH8(Msg); Msg += 8;
                    MsgLen &= 7;
            case 0: ;
        }
    }

    /* MsgLen == 0 to 7 */

    KOMI32_FINALIZE();

    return Seed1;
}

/*------------------------------------------------------------ */

static uint32_t Komi32(const void * in, const size_t len, const uint64_t seed) {
    return Komi32_impl(in, len, seed);
}

#undef GET_U32

#endif /* KOMI32_H */
