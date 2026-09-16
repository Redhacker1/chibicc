#ifndef _X86INTRIN_H_INCLUDED
#define _X86INTRIN_H_INCLUDED

/*
 * Minimal x86/x86-64 intrinsic compatibility header.
 *
 * Intended for:
 *   - chibicc
 *   - MinGW-w64 headers
 *   - Win32 / Win64
 *
 * This header intentionally avoids GCC-specific inline assembly and
 * compiler internals wherever possible.
 */

#ifndef __cplusplus
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;

#if defined(__x86_64__) || defined(_M_X64)
typedef unsigned long long uint64_t;
#else
typedef unsigned long long uint64_t;
#endif
#endif


/*
 * Basic MSVC-compatible integer types.
 */

#ifndef __int8
#define __int8 char
#endif

#ifndef __int16
#define __int16 short
#endif

#ifndef __int32
#define __int32 int
#endif

#ifndef __int64
#define __int64 long long
#endif


/*
 * 64-bit integer intrinsic helpers.
 */

#if !defined(__rdtsc) && !defined(__INTRINSIC_DEFINED___rdtsc)
#define __INTRINSIC_DEFINED___rdtsc
static __inline__ unsigned long long
__rdtsc(void)
{
    unsigned int lo;
    unsigned int hi;

#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__ (
        "rdtsc"
        : "=a"(lo), "=d"(hi)
    );

    return ((unsigned long long)hi << 32) | lo;
#else
    return 0;
#endif
}
#endif


/*
 * CPUID.
 */

#if !defined(__cpuid) && !defined(__INTRINSIC_DEFINED___cpuid)
#define __INTRINSIC_DEFINED___cpuid
static __inline__ void
__cpuid(
    int cpu_info[4],
    int leaf
)
{
#if defined(__GNUC__) || defined(__clang__)

    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax),
          "=b"(ebx),
          "=c"(ecx),
          "=d"(edx)
        : "a"((unsigned int)leaf)
    );

    cpu_info[0] = (int)eax;
    cpu_info[1] = (int)ebx;
    cpu_info[2] = (int)ecx;
    cpu_info[3] = (int)edx;

#else

    cpu_info[0] = 0;
    cpu_info[1] = 0;
    cpu_info[2] = 0;
    cpu_info[3] = 0;

#endif
}
#endif


#if !defined(__cpuidex) && !defined(__INTRINSIC_DEFINED___cpuidex)
#define __INTRINSIC_DEFINED___cpuidex
static __inline__ void
__cpuidex(
    int cpu_info[4],
    int leaf,
    int subleaf
)
{
#if defined(__GNUC__) || defined(__clang__)

    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;

    __asm__ __volatile__(
        "cpuid"
        : "=a"(eax),
          "=b"(ebx),
          "=c"(ecx),
          "=d"(edx)
        : "a"((unsigned int)leaf),
          "c"((unsigned int)subleaf)
    );

    cpu_info[0] = (int)eax;
    cpu_info[1] = (int)ebx;
    cpu_info[2] = (int)ecx;
    cpu_info[3] = (int)edx;

#else

    cpu_info[0] = 0;
    cpu_info[1] = 0;
    cpu_info[2] = 0;
    cpu_info[3] = 0;

#endif
}
#endif


/*
 * Memory barriers.
 */

#if !defined(_ReadWriteBarrier) && !defined(__INTRINSIC_DEFINED__ReadWriteBarrier)
#define __INTRINSIC_DEFINED__ReadWriteBarrier
static __inline__ void
_ReadWriteBarrier(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("" ::: "memory");
#endif
}
#endif


#if !defined(_mm_mfence) && !defined(__INTRINSIC_DEFINED__mm_mfence)
#define __INTRINSIC_DEFINED__mm_mfence
static __inline__ void
_mm_mfence(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("mfence" ::: "memory");
#endif
}
#endif


#if !defined(_mm_lfence) && !defined(__INTRINSIC_DEFINED__mm_lfence)
#define __INTRINSIC_DEFINED__mm_lfence
static __inline__ void
_mm_lfence(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("lfence" ::: "memory");
#endif
}
#endif


#if !defined(_mm_sfence) && !defined(__INTRINSIC_DEFINED__mm_sfence)
#define __INTRINSIC_DEFINED__mm_sfence
static __inline__ void
_mm_sfence(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("sfence" ::: "memory");
#endif
}
#endif


/*
 * Pause instruction.
 */

#if !defined(_mm_pause) && !defined(__INTRINSIC_DEFINED__mm_pause)
#define __INTRINSIC_DEFINED__mm_pause
static __inline__ void
_mm_pause(void)
{
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__("pause");
#endif
}
#endif


/*
 * Byte swap.
 */

#if !defined(_byteswap_ushort) && !defined(__INTRINSIC_DEFINED__byteswap_ushort)
#define __INTRINSIC_DEFINED__byteswap_ushort
static __inline__ unsigned short
_byteswap_ushort(unsigned short value)
{
    return (unsigned short)(
        ((value & 0x00ffu) << 8) |
        ((value & 0xff00u) >> 8)
    );
}
#endif


#if !defined(_byteswap_ulong) && !defined(__INTRINSIC_DEFINED__byteswap_ulong)
#define __INTRINSIC_DEFINED__byteswap_ulong
static __inline__ unsigned long
_byteswap_ulong(unsigned long value)
{
    return
        ((value & 0x000000ffUL) << 24) |
        ((value & 0x0000ff00UL) << 8)  |
        ((value & 0x00ff0000UL) >> 8)  |
        ((value & 0xff000000UL) >> 24);
}
#endif


#if !defined(_byteswap_uint64) && !defined(__INTRINSIC_DEFINED__byteswap_uint64)
#define __INTRINSIC_DEFINED__byteswap_uint64
static __inline__ unsigned long long
_byteswap_uint64(unsigned long long value)
{
    return
        ((value & 0x00000000000000ffULL) << 56) |
        ((value & 0x000000000000ff00ULL) << 40) |
        ((value & 0x0000000000ff0000ULL) << 24) |
        ((value & 0x00000000ff000000ULL) << 8)  |
        ((value & 0x000000ff00000000ULL) >> 8)  |
        ((value & 0x0000ff0000000000ULL) >> 24) |
        ((value & 0x00ff000000000000ULL) >> 40) |
        ((value & 0xff00000000000000ULL) >> 56);
}
#endif


/*
 * Bit scan.
 */

#if !defined(_BitScanForward) && !defined(__INTRINSIC_DEFINED__BitScanForward)
#define __INTRINSIC_DEFINED__BitScanForward
static __inline__ unsigned char
_BitScanForward(
    unsigned long *index,
    unsigned long mask
)
{
    unsigned long i;

    if (mask == 0)
        return 0;

    for (i = 0; i < sizeof(unsigned long) * 8; ++i)
    {
        if (mask & ((unsigned long)1 << i))
        {
            *index = i;
            return 1;
        }
    }

    return 0;
}
#endif


#if !defined(_BitScanReverse) && !defined(__INTRINSIC_DEFINED__BitScanReverse)
#define __INTRINSIC_DEFINED__BitScanReverse
static __inline__ unsigned char
_BitScanReverse(
    unsigned long *index,
    unsigned long mask
)
{
    unsigned long i;

    if (mask == 0)
        return 0;

    i = sizeof(unsigned long) * 8;

    while (i--)
    {
        if (mask & ((unsigned long)1 << i))
        {
            *index = i;
            return 1;
        }
    }

    return 0;
}
#endif


#if defined(__x86_64__) || defined(_M_X64)

#if !defined(_BitScanForward64) && !defined(__INTRINSIC_DEFINED__BitScanForward64)
#define __INTRINSIC_DEFINED__BitScanForward64
static __inline__ unsigned char
_BitScanForward64(
    unsigned long *index,
    unsigned long long mask
)
{
    unsigned long long i;

    if (mask == 0)
        return 0;

    for (i = 0; i < 64; ++i)
    {
        if (mask & (1ULL << i))
        {
            *index = (unsigned long)i;
            return 1;
        }
    }

    return 0;
}
#endif


#if !defined(_BitScanReverse64) && !defined(__INTRINSIC_DEFINED__BitScanReverse64)
#define __INTRINSIC_DEFINED__BitScanReverse64
static __inline__ unsigned char
_BitScanReverse64(
    unsigned long *index,
    unsigned long long mask
)
{
    unsigned long long i;

    if (mask == 0)
        return 0;

    i = 64;

    while (i--)
    {
        if (mask & (1ULL << i))
        {
            *index = (unsigned long)i;
            return 1;
        }
    }

    return 0;
}
#endif

#endif


/*
 * Rotate operations.
 */

#if !defined(_rotl) && !defined(__INTRINSIC_DEFINED__rotl)
#define __INTRINSIC_DEFINED__rotl
static __inline__ unsigned int
_rotl(
    unsigned int value,
    int shift
)
{
    unsigned int bits = sizeof(value) * 8;

    shift &= bits - 1;

    if (shift == 0)
        return value;

    return (value << shift) | (value >> (bits - shift));
}
#endif


#if !defined(_rotr) && !defined(__INTRINSIC_DEFINED__rotr)
#define __INTRINSIC_DEFINED__rotr
static __inline__ unsigned int
_rotr(
    unsigned int value,
    int shift
)
{
    unsigned int bits = sizeof(value) * 8;

    shift &= bits - 1;

    if (shift == 0)
        return value;

    return (value << shift) | (value >> (bits - shift));
}
#endif


#if !defined(_rotl64) && !defined(__INTRINSIC_DEFINED__rotl64)
#define __INTRINSIC_DEFINED__rotl64
static __inline__ unsigned long long
_rotl64(
    unsigned long long value,
    int shift
)
{
    shift &= 63;

    if (shift == 0)
        return value;

    return (value << shift) | (value >> (64 - shift));
}
#endif


#if !defined(_rotr64) && !defined(__INTRINSIC_DEFINED__rotr64)
#define __INTRINSIC_DEFINED__rotr64
static __inline__ unsigned long long
_rotr64(
    unsigned long long value,
    int shift
)
{
    shift &= 63;

    if (shift == 0)
        return value;

    return (value >> shift) | (value << (64 - shift));
}
#endif


/*
 * Compiler memory intrinsics commonly referenced by Windows headers.
 */

#if !defined(__stosb) && !defined(__INTRINSIC_DEFINED___stosb)
#define __INTRINSIC_DEFINED___stosb
static __inline__ void
__stosb(
    unsigned char *dst,
    unsigned char value,
    unsigned long long count
)
{
#if defined(__x86_64__) || defined(_M_X64)

    while (count--)
        *dst++ = value;

#else

    while (count--)
        *dst++ = value;

#endif
}
#endif


#if !defined(__stosw) && !defined(__INTRINSIC_DEFINED___stosw)
#define __INTRINSIC_DEFINED___stosw
static __inline__ void
__stosw(
    unsigned short *dst,
    unsigned short value,
    unsigned long long count
)
{
    while (count--)
        *dst++ = value;
}
#endif


#if !defined(__stosd) && !defined(__INTRINSIC_DEFINED___stosd)
#define __INTRINSIC_DEFINED___stosd
static __inline__ void
__stosd(
    unsigned long *dst,
    unsigned long value,
    unsigned long long count
)
{
    while (count--)
        *dst++ = value;
}
#endif


#if !defined(__movsb) && !defined(__INTRINSIC_DEFINED___movsb)
#define __INTRINSIC_DEFINED___movsb
static __inline__ void
__movsb(
    unsigned char *dst,
    const unsigned char *src,
    unsigned long long count
)
{
    while (count--)
        *dst++ = *src++;
}
#endif


#if !defined(__movsw) && !defined(__INTRINSIC_DEFINED___movsw)
#define __INTRINSIC_DEFINED___movsw
static __inline__ void
__movsw(
    unsigned short *dst,
    const unsigned short *src,
    unsigned long long count
)
{
    while (count--)
        *dst++ = *src++;
}
#endif


#if !defined(__movsd) && !defined(__INTRINSIC_DEFINED___movsd)
#define __INTRINSIC_DEFINED___movsd
static __inline__ void
__movsd(
    unsigned long *dst,
    const unsigned long *src,
    unsigned long long count
)
{
    while (count--)
        *dst++ = *src++;
}
#endif


/*
 * Read/write unaligned values.
 */

#if !defined(__readshort) && !defined(__INTRINSIC_DEFINED___readshort)
#define __INTRINSIC_DEFINED___readshort
static __inline__ unsigned short
__readshort(const void *address)
{
    const unsigned char *p = (const unsigned char *)address;

    return (unsigned short)(
        ((unsigned short)p[0]) |
        ((unsigned short)p[1] << 8)
    );
}
#endif


#if !defined(__readint) && !defined(__INTRINSIC_DEFINED___readint)
#define __INTRINSIC_DEFINED___readint
static __inline__ unsigned int
__readint(const void *address)
{
    const unsigned char *p = (const unsigned char *)address;

    return
        ((unsigned int)p[0]) |
        ((unsigned int)p[1] << 8) |
        ((unsigned int)p[2] << 16) |
        ((unsigned int)p[3] << 24);
}
#endif


#if !defined(__read64bit) && !defined(__INTRINSIC_DEFINED___read64bit)
#define __INTRINSIC_DEFINED___read64bit
static __inline__ unsigned long long
__read64bit(const void *address)
{
    const unsigned char *p = (const unsigned char *)address;

    return
        ((unsigned long long)p[0]) |
        ((unsigned long long)p[1] << 8) |
        ((unsigned long long)p[2] << 16) |
        ((unsigned long long)p[3] << 24) |
        ((unsigned long long)p[4] << 32) |
        ((unsigned long long)p[5] << 40) |
        ((unsigned long long)p[6] << 48) |
        ((unsigned long long)p[7] << 56);
}
#endif


#if !defined(__writebyte) && !defined(__INTRINSIC_DEFINED___writebyte)
#define __INTRINSIC_DEFINED___writebyte
static __inline__ void
__writebyte(
    unsigned char *address,
    unsigned char value
)
{
    *address = value;
}
#endif


#if !defined(__writeshort) && !defined(__INTRINSIC_DEFINED___writeshort)
#define __INTRINSIC_DEFINED___writeshort
static __inline__ void
__writeshort(
    unsigned short *address,
    unsigned short value
)
{
    *address = value;
}
#endif


#if !defined(__writeint) && !defined(__INTRINSIC_DEFINED___writeint)
#define __INTRINSIC_DEFINED___writeint
static __inline__ void
__writeint(
    unsigned int *address,
    unsigned int value
)
{
    *address = value;
}
#endif


#if !defined(__write64bit) && !defined(__INTRINSIC_DEFINED___write64bit)
#define __INTRINSIC_DEFINED___write64bit
static __inline__ void
__write64bit(
    unsigned long long *address,
    unsigned long long value
)
{
    *address = value;
}
#endif


#endif /* _X86INTRIN_H_INCLUDED */
