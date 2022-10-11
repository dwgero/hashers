/*
 * Combo32 version 1.1
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
 * other portable 32-bit hashers in SMHasher3, using Komi32 for
 * byte strings < 64 and Mult32 for byte strings >= 64.
 */

#ifndef COMBO32_H
#define COMBO32_H

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

#include "mult32.h"
#include "komi32.h"

static uint32_t Combo32(const void * in, const size_t len, const uint64_t seed) {
    if (likely(len < 64))
        return Komi32(in, len, seed);
    return Mult32(in, len, seed);
}

#endif /* COMBO32_H */
