/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _MA_COMPILER_H_
#define _MA_COMPILER_H_

#define MA_LITTLE_ENDIAN (0x12u)
#define MA_BIG_ENDIAN    (0x21u)

// TODO refactor since __attribute__ is supported across many compiler
#if defined(__GNUC__)
    #define MA_ATTR_ALIGNED(Bytes)    __attribute__((aligned(Bytes)))
    #define MA_ATTR_SECTION(sec_name) __attribute__((section(#sec_name)))
    #define MA_ATTR_PACKED            __attribute__((packed))
    #define MA_ATTR_WEAK              __attribute__((weak))
    #define MA_ATTR_ALWAYS_INLINE     __attribute__((always_inline))
    #define MA_ATTR_DEPRECATED(mess)  __attribute__((deprecated(mess)))  // warn if function with this attribute is used
    #define MA_ATTR_UNUSED            __attribute__((unused))  // Function/Variable is meant to be possibly unused
    #define MA_ATTR_USED              __attribute__((used))    // Function/Variable is meant to be used

    #define MA_ATTR_PACKED_BEGIN
    #define MA_ATTR_PACKED_END
    #define MA_ATTR_BIT_FIELD_ORDER_BEGIN
    #define MA_ATTR_BIT_FIELD_ORDER_END

    #if __has_attribute(__fallthrough__)
        #define MA_ATTR_FALLTHROUGH __attribute__((fallthrough))
    #else
        #define MA_ATTR_FALLTHROUGH \
            do {                    \
            } while (0) /* fallthrough */
    #endif

    // Endian conversion use well-known host to network (big endian) naming
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define MA_BYTE_ORDER MA_LITTLE_ENDIAN
    #else
        #define MA_BYTE_ORDER MA_BIG_ENDIAN
    #endif

    #define MA_BSWAP16(u16) (__builtin_bswap16(u16))
    #define MA_BSWAP32(u32) (__builtin_bswap32(u32))

    #ifndef __ARMCC_VERSION
        // List of obsolete callback function that is renamed and should not be defined.
        // Put it here since only gcc support this pragma
        #pragma GCC poison tud_vendor_control_request_cb
    #endif

#elif defined(__TI_COMPILER_VERSION__)
    #define MA_ATTR_ALIGNED(Bytes)    __attribute__((aligned(Bytes)))
    #define MA_ATTR_SECTION(sec_name) __attribute__((section(#sec_name)))
    #define MA_ATTR_PACKED            __attribute__((packed))
    #define MA_ATTR_WEAK              __attribute__((weak))
    #define MA_ATTR_ALWAYS_INLINE     __attribute__((always_inline))
    #define MA_ATTR_DEPRECATED(mess)  __attribute__((deprecated(mess)))  // warn if function with this attribute is used
    #define MA_ATTR_UNUSED            __attribute__((unused))  // Function/Variable is meant to be possibly unused
    #define MA_ATTR_USED              __attribute__((used))
    #define MA_ATTR_FALLTHROUGH       __attribute__((fallthrough))

    #define MA_ATTR_PACKED_BEGIN
    #define MA_ATTR_PACKED_END
    #define MA_ATTR_BIT_FIELD_ORDER_BEGIN
    #define MA_ATTR_BIT_FIELD_ORDER_END

    // __BYTE_ORDER is defined in the TI ARM compiler, but not MSP430 (which is
    // little endian)
    #if ((__BYTE_ORDER__) == (__ORDER_LITTLE_ENDIAN__)) || defined(__MSP430__)
        #define MA_BYTE_ORDER MA_LITTLE_ENDIAN
    #else
        #define MA_BYTE_ORDER MA_BIG_ENDIAN
    #endif

    #define MA_BSWAP16(u16) (__builtin_bswap16(u16))
    #define MA_BSWAP32(u32) (__builtin_bswap32(u32))

#elif defined(__ICCARM__)
    #include <intrinsics.h>

    #define MA_ATTR_ALIGNED(Bytes)    __attribute__((aligned(Bytes)))
    #define MA_ATTR_SECTION(sec_name) __attribute__((section(#sec_name)))
    #define MA_ATTR_PACKED            __attribute__((packed))
    #define MA_ATTR_WEAK              __attribute__((weak))
    #define MA_ATTR_ALWAYS_INLINE     __attribute__((always_inline))
    #define MA_ATTR_DEPRECATED(mess)  __attribute__((deprecated(mess)))  // warn if function with this attribute is used
    #define MA_ATTR_UNUSED            __attribute__((unused))  // Function/Variable is meant to be possibly unused
    #define MA_ATTR_USED              __attribute__((used))    // Function/Variable is meant to be used
    #define MA_ATTR_FALLTHROUGH       __attribute__((fallthrough))

    #define MA_ATTR_PACKED_BEGIN
    #define MA_ATTR_PACKED_END
    #define MA_ATTR_BIT_FIELD_ORDER_BEGIN
    #define MA_ATTR_BIT_FIELD_ORDER_END

    // Endian conversion use well-known host to network (big endian) naming
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define MA_BYTE_ORDER MA_LITTLE_ENDIAN
    #else
        #define MA_BYTE_ORDER MA_BIG_ENDIAN
    #endif

    #define MA_BSWAP16(u16) (__iar_builtin_REV16(u16))
    #define MA_BSWAP32(u32) (__iar_builtin_REV(u32))

#elif defined(__CCRX__)
    #define MA_ATTR_ALIGNED(Bytes)
    #define MA_ATTR_SECTION(sec_name)
    #define MA_ATTR_PACKED
    #define MA_ATTR_WEAK
    #define MA_ATTR_ALWAYS_INLINE
    #define MA_ATTR_DEPRECATED(mess)
    #define MA_ATTR_UNUSED
    #define MA_ATTR_USED
    #define MA_ATTR_FALLTHROUGH \
        do {                    \
        } while (0) /* fallthrough */

    #define MA_ATTR_PACKED_BEGIN          _Pragma("pack")
    #define MA_ATTR_PACKED_END            _Pragma("packoption")
    #define MA_ATTR_BIT_FIELD_ORDER_BEGIN _Pragma("bit_order right")
    #define MA_ATTR_BIT_FIELD_ORDER_END   _Pragma("bit_order")

    // Endian conversion use well-known host to network (big endian) naming
    #if defined(__LIT)
        #define MA_BYTE_ORDER MA_LITTLE_ENDIAN
    #else
        #define MA_BYTE_ORDER MA_BIG_ENDIAN
    #endif

    #define MA_BSWAP16(u16) ((unsigned short)_builtin_revw((unsigned long)u16))
    #define MA_BSWAP32(u32) (_builtin_revl(u32))

#else
    #error "Compiler attribute porting is required"
#endif

#if (MA_BYTE_ORDER == MA_LITTLE_ENDIAN)

    #define ma_htons(u16)   (MA_BSWAP16(u16))
    #define ma_ntohs(u16)   (MA_BSWAP16(u16))

    #define ma_htonl(u32)   (MA_BSWAP32(u32))
    #define ma_ntohl(u32)   (MA_BSWAP32(u32))

    #define ma_htole16(u16) (u16)
    #define ma_le16toh(u16) (u16)

    #define ma_htole32(u32) (u32)
    #define ma_le32toh(u32) (u32)

#elif (MA_BYTE_ORDER == MA_BIG_ENDIAN)

    #define ma_htons(u16)   (u16)
    #define ma_ntohs(u16)   (u16)

    #define ma_htonl(u32)   (u32)
    #define ma_ntohl(u32)   (u32)

    #define ma_htole16(u16) (MA_BSWAP16(u16))
    #define ma_le16toh(u16) (MA_BSWAP16(u16))

    #define ma_htole32(u32) (MA_BSWAP32(u32))
    #define ma_le32toh(u32) (MA_BSWAP32(u32))

#else
    #error Byte order is undefined
#endif

#endif
