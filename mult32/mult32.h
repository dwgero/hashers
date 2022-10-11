/*
 * Mult32 version 1.4
 * Copyright (c) 2022 David W. Gero
 *
 * This file is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This 32-bit hasher is portable, uses no special CPU instructions,
 * passes all the tests of SMHasher3, and is perfect for hash tables
 * with up to 4 billion buckets.
 * It is the fastest portable 32-bit hasher as tested against all the
 * other portable 32-bit hashers in SMHasher3, achieving average
 * speeds of 7.7 to 8.0 bytes per cycle running on a 2.6 GHz processor
 * in a system from 2016.
 */

#ifndef mult32_h
#define mult32_h

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

/* RANDOM_POWER must be a power of 2 */
#define RANDOM_POWER 128
#define RANDOM_EXTRA 9
#define RANDOM_LENGTH (RANDOM_POWER + RANDOM_EXTRA)
static uint64_t mult32_random[RANDOM_LENGTH];

/*------------------------------------------------------------ */

/* Mult32 helpers */

/* Can be called with MsgLen == 0 to 7
 * This is the only part of the algorithm where it matters
 * if the processor is little-endian or big-endian
 */
static inline uint64_t final_bytes(const uint64_t* const Msg, const size_t MsgLen) {
    #if defined(LITTLE_ENDIAN)
        /* A modified version of 0xDEADBEEFDEADBEEF */
        uint64_t m = UINT64_C(0xDCADBCEDDCADBCED);
    #else
        /* BIG_ENDIAN */
        uint64_t m = UINT64_C(0xEDBCADDCEDBCADDC); /* byte-swapped */
    #endif
    
        memcpy(&m, Msg, MsgLen);
    #if defined(LITTLE_ENDIAN)
      #if defined(NO_SWAP)
        return m;
      #else
        return BSWAP64(m);
      #endif
    #else
      /* BIG_ENDIAN */
      #if defined(NO_SWAP)
        return BSWAP64(m);
      #else
        return m;
      #endif
    #endif
}

/* Need to be able to multiply two 32-bit numbers and get a 64-bit result.
 * Note that m2 is always some uint64_t value >> 32, so leave it alone.
 */
static inline uint64_t mult32_m64(const uint32_t m1, const uint64_t m2) {
    return (uint64_t)m1 * m2;
}

/* Hashing round reading 8-byte input.
 * My experiments did not find a faster hashing round that is portable,
 * uses no CPU-specific instructions, and passes all SMHasher3 tests.
 * Cost:
 *   1 possibly unaligned 64-bit memory read
 *   1 aligned 64-bit array read
 *   2 post-increments
 *   2 exclusive-ors
 *   1 right shift by a constant 32
 *   1 32-bit by 32-bit multiply with 64-bit result
 */
#if defined(ALLOW_UNLAIGNED_READS) && ALLOW_UNALIGNED_READS
  #if defined(NO_SWAP)
    #define MULT32_HASH8(m,i) do { \
        uint64_t val = *m++ ^ mult32_random[i++]; \
\
        hash ^= mult32_m64((uint32_t)val, val >> 32); \
    } while (0)

  #else

    #define MULT32_HASH8(m,i) do { \
        uint64_t val = BSWAP64(*m++) ^ mult32_random[i++]; \
\
        hash ^= mult32_m64((uint32_t)val, val >> 32); \
    } while (0)
  #endif /* defined(NO_SWAP) */

#else

  #if defined(NO_SWAP)
    #define MULT32_HASH8(m,i) do { \
        uint64_t val; \
\
        memcpy(&val, m++, 8); \
        val ^= mult32_random[i++]; \
        hash ^= mult32_m64((uint32_t)val, val >> 32); \
    } while (0)

  #else

    #define MULT32_HASH8(m,i) do { \
        uint64_t val; \
\
        memcpy(&val, m++, 8); \
        val = BSWAP64(val) ^ mult32_random[i++]; \
        hash ^= mult32_m64((uint32_t)val, val >> 32); \
    } while (0)
  #endif /* defined(NO_SWAP) */
#endif /* defined(ALLOW_UNALIGNED_READS) */

/* Hashing round without input */
#define MULT32_HASHROUND(i) do { \
    uint64_t val = mult32_random[i++]; \
\
    hash ^= mult32_m64((uint32_t)val, val >> 32); \
} while (0)

/* Value hashing round with 8-byte input */
#define MULT32_VALHASH8(x,i) do { \
    uint64_t val = (x) ^ mult32_random[i++]; \
\
    hash ^= mult32_m64((uint32_t)val, val >> 32); \
} while (0)

/* Seed hashing round with 4-byte input.
 * 32-bit implementation derived from Aleksey Vaneev's komihash
 * random number generator.
 */
#define KOMI32_SEEDHASH4(x) do { \
    uint64_t prod = mult32_m64(Seed1 ^ ((x) & UINT32_C(0x55555555)), \
                               Seed5 ^ ((x) & UINT32_C(0xAAAAAAAA))); \
\
    Seed5 += (uint32_t)(prod >> 32); \
    Seed1  = Seed5 ^ (uint32_t)prod; \
} while (0);

/* Handle reading final 0 to 7 bytes */
#define MULT32_FINALIZE(msg,len,i) do { \
    const uint64_t final = final_bytes(msg, len); \
\
    MULT32_VALHASH8(final, i); \
} while (0)

/*------------------------------------------------------------ */

/* Mult32 hash function */

static inline uint32_t Mult32_impl(const void* const in, const size_t len, const uint64_t UseSeed) {
    /* Allow the compiler to put Msg in a register */
    const uint64_t* Msg = (const uint64_t*)in;
    /* Allow the compiler to put MsgLen in a register */
    uint64_t MsgLen = (uint64_t)len;
    uint64_t hash = UseSeed ^ MsgLen;
    uint64_t i = ((MsgLen >> 6) ^ MsgLen) & (RANDOM_POWER - 1);
    
    /* Guaranteed to start with i < RANDOM_POWER */
    MULT32_VALHASH8(hash, i); /* now i < RANDOM_POWER + 1 */
    
    while (likely(MsgLen >= 64)) {
        prefetch(Msg);
        
        MULT32_HASH8(Msg, i); /* now i < RANDDOM_POWER + 2 */
        MULT32_HASH8(Msg, i); /* + 3 */
        MULT32_HASH8(Msg, i); /* + 4 */
        MULT32_HASH8(Msg, i); /* + 5 */
        MULT32_HASH8(Msg, i); /* + 6 */
        MULT32_HASH8(Msg, i); /* + 7 */
        MULT32_HASH8(Msg, i); /* + 8 */
        MULT32_HASH8(Msg, i); /* + 9 */
        MULT32_HASHROUND(i);  /* now i < RANDOM_POWER + 10, so the
                               * i used by MULT32_HASHROUND
                               * was < RANDOM_POWER + 9, which
                               * is why the mult32_random[] array
                               * is 9 longer than necessary */
        
        i &= RANDOM_POWER - 1;
        MsgLen -= 64;
    }
    
    /* MsgLen == 0 to 63 */
    
    prefetch(Msg);
    
    {
        uint8_t numHash8 = (MsgLen >> 3) & 7;
        
        /* Guaranteed to start with i < RANDOM_POWER + 1 */
        switch (numHash8) {
            case 7: MULT32_HASH8(Msg, i); /* now i < RANDOM_POWER + 2 */
            case 6: MULT32_HASH8(Msg, i); /* + 3 */
            case 5: MULT32_HASH8(Msg, i); /* + 4 */
            case 4: MULT32_HASH8(Msg, i); /* + 5 */
            case 3: MULT32_HASH8(Msg, i); /* + 6 */
            case 2: MULT32_HASH8(Msg, i); /* + 7 */
            case 1: MULT32_HASH8(Msg, i); /* + 8 */
                    MULT32_HASHROUND(i);  /* + 9 */
                    MsgLen &= 7;
            case 0: ;
        }
    }
    
    /* MsgLen == 0 to 7 */
    
    MULT32_FINALIZE(Msg, MsgLen, i); /* now i < RANDOM_POWER + 10, because
                                      * MULT32_FINALIZE does one last
                                      * MULT32_VALHASH8
                                      */
    
    /* Hash 64 bits down to 32.
     * This is fast, portable, and suitable for any 64-bit value,
     * including doubles and long ints.
     */
    {
        uint32_t Seed1 = UINT32_C(0xC5A308D3);
        uint32_t Seed5 = UINT32_C(0xB8D01377);
        
        KOMI32_SEEDHASH4((uint32_t)hash);
        KOMI32_SEEDHASH4((uint32_t)(hash >> 32));
        
        return Seed1;
    }
}

/*------------------------------------------------------------*/

/* An implementation of Sebastiano Vigna's Xorshift+,
 * which passes all BigCrush tests
 */

struct Xorshift128p_state {
    uint64_t x[2];
};

/* One round of SplitMix64 */
static inline uint64_t SplitMix64(const uint64_t x) {
    uint64_t result;
    
    result = (x      ^ (x      >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    result = (result ^ (result >> 27)) * UINT64_C(0x94D049BB133111EB);
    return    result ^ (result >> 31);
}

/* Intialize the Xorshift+ state using SplitMix64, which
 * was suggested as a good initializer by Vigna
 */
static struct Xorshift128p_state Xorshift128p_init(const uint64_t seed) {
    struct Xorshift128p_state result;
    uint64_t x = seed;
    const uint64_t y = UINT64_C(0x9E3779B97f4A7C15);
    
    x += y;
    result.x[0] = SplitMix64(x);
    x += y;
    result.x[1] = SplitMix64(x);
    
    return result;
}

/* One round of Xorshift+ */
static inline uint64_t Xorshift128p(struct Xorshift128p_state *state) {
    uint64_t x0 = state->x[0];
    uint64_t x1 = state->x[1];
    
    state->x[0] = x1;
    
    /* These shifts are tunable */
    x0 ^=       x0 << 23;  /* a */
    x0 ^=       x0 >> 18;  /* b */
    x0 ^= x1 ^ (x1 >>  5); /* c */
    
    state->x[1] = x0;
    
    return x0 + x1;
}

/*------------------------------------------------------------ */

/* Initialize the mult32_random[] array with random numbers.
 * This only happens once for the lifetime of the program,
 * so speed is not important.
 */
static void Mult32_init(void) {
    const uint64_t seed = UINT64_C(0xDEADBEEFDEADBEEF);
    struct Xorshift128p_state state = Xorshift128p_init(seed);
    
    for (unsigned int i = 0; i < RANDOM_LENGTH; i++) {
        mult32_random[i] = Xorshift128p(&state);
    }
}

/*------------------------------------------------------------ */

static uint32_t Mult32(const void* in, const size_t len, const uint64_t seed) {
    /* Force Mult32_init() to happen before anything else,
     * at a cost of one test for every hash call.
     * Alternatively, remove this test and call Mult32_init()
     * at the beginning of your program.
     */
    static int oneTimeDone = 0;
    if (unlikely(oneTimeDone == 0)) {
        Mult32_init();
        oneTimeDone = 1;
    }
    
    return Mult32_impl(in, len, seed);
}

#endif /* mult32_h */
