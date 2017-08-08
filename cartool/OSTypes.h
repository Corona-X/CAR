/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* OSTypes.h - System Types for all of the Corona-X System         */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* beeselmane - 27.11.2015 - 8:00 PM EST                           */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* Note: This file does not follow standard prefix rules           */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/

#ifndef __SYSTEM_OSTYPES__
#define __SYSTEM_OSTYPES__ 1

#include "Corona-X.h"

#define OSReflectMacro(...)                     __VA_ARGS__

#if !kCXAssemblyCode

#define OSEnum(...) __OSDeclareEnum(__VA_ARGS__, __OSDeclareEnumWithName, __OSDeclareEnumWithoutName)(__VA_ARGS__)
#define __OSDeclareEnumWithName(type, name)     type name; enum
#define __OSDeclareEnumWithoutName(type)        enum
#define __OSDeclareEnum(a, b, function, ...)    function

#define OSBufferMake(a, s)      ((OSBuffer){((OSSize)(s)), ((OSAddress)(a))})
#define OSBufferIsEmpty(b)      (!b.size)
#define kOSBufferEmpty          ((OSBuffer){kOSNullPointer, 0})

#define OSVAStart               __builtin_va_start
#define OSVAGetNext             __builtin_va_arg
#define OSVACopy                __builtin_va_copy
#define OSVAFinish              __builtin_va_end

#if kCXLanguageCXX
    #define kOSNullPointer      __null
#else /* Regular C */
    #define kOSNullPointer      ((OSAddress)0x0000000000000000)

    #define true                1
    #define false               0

    typedef _Bool               bool;
#endif /* C/C++ */

#if defined(__SIZEOF_INT128__) && (__SIZEOF_INT128__ == 16)
    #define kOSInt128Available  1

    typedef unsigned __int128   UInt128;
    typedef   signed __int128   SInt128;
#else /* unsupported */
    #define kOSInt128Available  0
#endif /* __int128 support */

typedef __builtin_va_list       OSVAList;

typedef unsigned long           UInt64;
typedef unsigned int            UInt32;
typedef unsigned short          UInt16;
typedef unsigned char           UInt8;

typedef   signed long           SInt64;
typedef   signed int            SInt32;
typedef   signed short          SInt16;
typedef   signed char           SInt8;

typedef UInt64                  UIntN;
typedef SInt64                  SIntN;

typedef bool                    OSBoolean;
typedef double                  OSFloat64;
typedef float                   OSFloat32;
typedef SIntN                   OSInteger;
typedef UIntN                   OSPointerValue;
typedef void                   *OSAddress;
typedef void                   *OSFuncPtr;
typedef void                   *OSPointer;
typedef SInt64                  OSOffset;
typedef OSOffset                OSIndex;
typedef UInt64                  OSSize;
typedef OSSize                  OSLength;
typedef OSSize                  OSCount;

typedef UInt32                  OSUnicodePoint;
typedef UInt32                  OSUTF32Char;
typedef UInt16                  OSUTF16Char;
typedef UInt8                   OSUTF8Char;

typedef struct {
    OSSize    size;
    OSAddress address;
} OSBuffer;

#endif /* !kCXAssemblyCode */

#endif /* !defined(__SYSTEM_OSTYPES__) */
