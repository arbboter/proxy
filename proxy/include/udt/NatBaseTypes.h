// NatBaseTypes.h: interface for the NatBaseTypes.
//
//////////////////////////////////////////////////////////////////////

#ifndef _NAT_BASETYPES_H_
#define _NAT_BASETYPES_H_


#include <stddef.h>  // for NULL, size_t

#if !(defined(_MSC_VER) && (_MSC_VER < 1600))
#include <stdint.h>  // for uintptr_t
#endif

#if !defined(INT_TYPES_DEFINED)

#define INT_TYPES_DEFINED

#ifdef COMPILER_MSVC
typedef unsigned __int64 uint64;
typedef __int64 int64;
#ifndef INT64_C
#define INT64_C(x) x ## I64
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## UI64
#endif
#define INT64_F "I64"
#else  // COMPILER_MSVC
// On Mac OS X, cssmconfig.h defines uint64 as uint64_t
// TODO(fbarchard): Use long long for compatibility with chromium on BSD/OSX.
#if defined(OSX)
typedef uint64_t uint64;
typedef int64_t int64;
#ifndef INT64_C
#define INT64_C(x) x ## LL
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## ULL
#endif
#define INT64_F "l"
#elif defined(__LP64__)
typedef unsigned long uint64;  // NOLINT
typedef long int64;  // NOLINT
#ifndef INT64_C
#define INT64_C(x) x ## L
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## UL
#endif
#define INT64_F "l"
#else  // __LP64__
typedef unsigned long long uint64;  // NOLINT
typedef long long int64;  // NOLINT
#ifndef INT64_C
#define INT64_C(x) x ## LL
#endif
#ifndef UINT64_C
#define UINT64_C(x) x ## ULL
#endif
#define INT64_F "ll"
#endif  // __LP64__
#endif  // COMPILER_MSVC
typedef unsigned int uint32;
typedef int int32;
typedef unsigned short uint16;  // NOLINT
typedef short int16;  // NOLINT
typedef unsigned char uint8;
typedef signed char int8;
typedef uint8 byte;

#endif  // INT_TYPES_DEFINED

// Detect compiler is for x86 or x64.
#if defined(__x86_64__) || defined(_M_X64) || \
	defined(__i386__) || defined(_M_IX86)
#define CPU_X86 1
#endif
// Detect compiler is for arm.
#if defined(__arm__) || defined(_M_ARM)
#define CPU_ARM 1
#endif
#if defined(CPU_X86) && defined(CPU_ARM)
#error CPU_X86 and CPU_ARM both defined.
#endif
#if !defined(ARCH_CPU_BIG_ENDIAN) && !defined(ARCH_CPU_LITTLE_ENDIAN)
// x86, arm or GCC provided __BYTE_ORDER__ macros
#if CPU_X86 || CPU_ARM ||  \
	(defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define ARCH_CPU_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ARCH_CPU_BIG_ENDIAN
#else
#error ARCH_CPU_BIG_ENDIAN or ARCH_CPU_LITTLE_ENDIAN should be defined.
#endif
#endif
#if defined(ARCH_CPU_BIG_ENDIAN) && defined(ARCH_CPU_LITTLE_ENDIAN)
#error ARCH_CPU_BIG_ENDIAN and ARCH_CPU_LITTLE_ENDIAN both defined.
#endif

#endif//_NAT_BASETYPES_H_