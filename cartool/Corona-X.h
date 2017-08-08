/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* Corona-X.h - Standard defines for Corona-X                      */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* beeselmane - 20.8.2015  - ?:?? ?? ???                           */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/

#ifndef __CORONA_X__
#define __CORONA_X__ 1

#ifdef __x86_64__
    #define kCXLittleEndian         1
    #define kCXBigEndian            0
    #define kCXEmbedded             0

    #define kCXArchX86_64           1
    #define kCXArchI386             0
    #define kCXArchIA               1
    #define kCXArchARM64            0
    #define kCXArchARMv7            0
    #define kCXArchARM              0
#elif defined(__i386__)
    #define kCXLittleEndian         1
    #define kCXBigEndian            0
    #define kCXEmbedded             0

    #define kCXArchX86_64           0
    #define kCXArchI386             1
    #define kCXArchIA               1
    #define kCXArchARM64            0
    #define kCXArchARMv7            0
    #define kCXArchARM              0
#elif defined(__arm64__) && defined(__ARM_ARCH) && (__ARM_ARCH == 8)
    #define kCXLittleEndian         1
    #define kCXBigEndian            0
    #define kCXEmbedded             1

    #define kCXArchX86_64           0
    #define kCXArchI386             0
    #define kCXArchIA               0
    #define kCXArchARM64            1
    #define kCXArchARMv7            0
    #define kCXArchARM              1
#elif defined(__arm__) && defined(__ARM_ARCH) && (__ARM_ARCH == 7)
    #define kCXLittleEndian         1
    #define kCXBigEndian            0
    #define kCXEmbedded             1

    #define kCXArchX86_64           0
    #define kCXArchI386             0
    #define kCXArchIA               0
    #define kCXArchARM64            0
    #define kCXArchARMv7            1
    #define kCXArchARM              1
#else /* Unsupported */
    #error Unsupported Architecture!
#endif /* Target Architecture */

#if defined(__LP64__) || defined(_LP64)
    #define kCXABI32Bit             0
    #define kCXABI64Bit             1
#else /* LP32 */
    #define kCXABI32Bit             1
    #define kCXABI64Bit             0
#endif /* Target ABI */

#ifdef __CORONA__
    #define kCXTargetOSCorona       1
    #define kCXTargetOSApple        0
    #define kCXTargetOSWindows      0
    #define kCXTargetOSLinux        0

    #define kCXHostOSCorona         1
    #define kCXHostOSApple          0
    #define kCXHostOSWindows        0
    #define kCXHostOSLinux          0
#elif defined(__APPLE__) || defined(__APPLE_CC__)
    #define kCXTargetOSCorona       1
    #define kCXTargetOSApple        0
    #define kCXTargetOSWindows      0
    #define kCXTargetOSLinux        0

    #define kCXHostOSCorona         0
    #define kCXHostOSApple          1
    #define kCXHostOSWindows        0
    #define kCXHostOSLinux          0
#elif defined(_WIN32)
    #define kCXTargetOSCorona       1
    #define kCXTargetOSApple        0
    #define kCXTargetOSWindows      0
    #define kCXTargetOSLinux        0

    #define kCXHostOSCorona         0
    #define kCXHostOSApple          0
    #define kCXHostOSWindows        1
    #define kCXHostOSLinux          0
#elif defined(__linux) || defined(__unix)
    #define kCXTargetOSCorona       1
    #define kCXTargetOSApple        0
    #define kCXTargetOSWindows      0
    #define kCXTargetOSLinux        0

    #define kCXHostOSCorona         0
    #define kCXHostOSApple          0
    #define kCXHostOSWindows        0
    #define kCXHostOSLinux          1
#else /* Other; Assume ELF (Linux) */
    #define kCXTargetOSCorona       1
    #define kCXTargetOSApple        0
    #define kCXTargetOSWindows      0
    #define kCXTargetOSLinux        0

    #define kCXHostOSCorona         0
    #define kCXHostOSApple          0
    #define kCXHostOSWindows        0
    #define kCXHostOSLinux          1
#endif /* Target + Host OS */

#ifdef __posix
    #define kCXFullPOSIX            1
#else /* No POSIX */
    #define kCXFullPOSIX            0
#endif /* POSIX Detection */

#ifdef __clang__
    #define kCXCompilerGCCLike      1
    #define kCXCompilerClang        1
    #define kCXCompilerGCC          0
    #define kCXCompilerMSC          0

    #define kCXCompilerName         "Clang"
#elif defined(__gcc__) || defined(__GNUC__)
    #define kCXCompilerGCCLike      1
    #define kCXCompilerClang        0
    #define kCXCompilerGCC          1
    #define kCXCompilerMSC          0

    #define kCXCompilerName         "GCC"
#elif defined(_MSC_VER)
    #define kCXCompilerGCCLike      0
    #define kCXCompilerClang        0
    #define kCXCompilerGCC          0
    #define kCXCompilerMSC          1

    #define kCXCompilerName         "Microsoft C Compiler"
#else /* Unsupported */
    #error Unsupported Compiler!
#endif /* Compiler */

#define kCXSTDCXXVersionCXX14       201402L
#define kCXSTDCXXVersionCXX11       201103L

#define kCXSTDCVersionC11           201112L
#define kCXSTDCVersionC99           199901L
#define kCXSTDCVersionC98_e1        199409L

#ifdef __cplusplus
    #define kCXLanguageC            0
    #define kCXLanguageCXX          1
    #define kCXLanguageVersion      __cplusplus

    #define kCXLanguageCXX11        (kCXLanguageVersion >= kCXSTDCXXVersionCXX11)
    #define kCXLanguageCXX14        (kCXLanguageVersion >= kCXSTDCXXVersionCXX14)

    #define kCXLanguageC98          0
    #define kCXLanguageC99          0
    #define kCXLanguageC11          0
#else /* Regular C */
    #define kCXLanguageC            1
    #define kCXLanguageCXX          0
    #define kCXLanguageVersion      __STDC_VERSION__

    #define kCXLanguageCXX11        0
    #define kCXLanguageCXX14        0

    #define kCXLanguageC98          (kCXLanguageVersion >= kCXSTDCVersionC98_e1)
    #define kCXLanguageC99          (kCXLanguageVersion >= kCXSTDCVersionC99)
    #define kCXLanguageC11          (kCXLanguageVersion >= kCXSTDCVersionC11)
#endif /* Language Detection */

#ifdef __GNUC__
    #define kCXLanguageGNUC         1
    #define kCXGNUCVersion          __GNUC__
#else /* Standard C */
    #define kCXLanguageGNUC         0
    #define kCXGNUCVersion          0
#endif

#ifdef __OBJC
    #define kCXLanguageObjC         1

    #ifdef __OBJC2
        #define kCXObjC2            1
    #endif /* Objective-C 2 */
#else /* Regular C */
    #define kCXLanguageObjC         0
    #define kCXObjC2                0
#endif /* Objective-C */

// Note: If preprocessing an assembly file,
// compile with -DkCXAssemblyCode=1 to ensure
// that no C definitions are included in the output
#ifndef kCXAssemblyCode
    #define kCXAssemblyCode         0
#endif /* Assembly Code */

// Note: When the bootloader is compiling,
// this flag will be set to 1 so that the
// headers know to apply fixes to the EDK2
// build toolchain
#ifndef kCXBootloaderCode
    #define kCXBootloaderCode       0
#endif /* Bootloader Code */

#if !(defined(kCXDebug) || defined(kCXDevelopment) || defined(kCXRelease))
    #define kCXDebug                0
    #define kCXDevelopment          1
    #define kCXRelease              0

    #warning Assuming Build Type is Development!
#endif /* Build Type */

#ifndef kCXDebug
    #define kCXDebug                0
#endif /* Not Debug */

#ifndef kCXDevelopment
    #define kCXDevelopment          0
#endif /* Not Development */

#ifndef kCXRelease
    #define kCXRelease              0
#endif /* Not Release */

#ifndef kCXBuildDev
    #define kCXBuildDev (kCXDebug || kCXDevelopment)
#endif /* Build Development Functionality */

#ifdef __has_feature
    #define CXHasFeature            __has_feature
#else /* !defined(__has_feature) */
    #define CXHasFeature(...)       0
#endif /* Compiler Feature Detection */

#ifdef __has_attribute
    #define CXHasAttribue           __has_attribute
#else /* !defined(__has_attribute) */
    #define CXHasAttribue(...)      0
#endif /* Compiler Attribute Detection */

#ifdef __has_cpp_attribute
    #define CXHasCXXAttribute       __has_cpp_attribute
#else /* !defined(__has_cpp_attribute) */
    #define CXHasCXXAttribute(...)  0
#endif /* Compiler C++ Attribute Detection */

#ifdef __has_extension
    #define CXHasExtension          __has_extension
#else /* !defined(__has_extension) */
    #define CXHasExtension(e)       CXHasFeature(e)
#endif /* Compiler Extension Detection */

#ifdef __has_builtin
    #define CXHasBuiltin            __has_builtin
#else /* !defined(__has_builtin) */
    #define CXHasBuiltIn(...)       0
#endif /* Compiler BuiltIn Detection */

#ifdef __has_include
    #define CXHasInclude            __has_include
#else  /* !defined(__has_include) */
    #define CXHasInclude(...)       0
#endif /* Include file checking */

#ifdef __has_include_next
    #define CXHasIncludeNext        __has_include_next
#else  /* !defined(__has_include_next) */
    #define CXHasIncludeNext(...)   0
#endif /* Include file checking */

#ifdef __BASE_FILE__
    #define kCXBaseFile             __BASE_FILE__
#else /* !defined(__BASE_FILE__) */
    #define kCXBaseFile             "Unknown"
#endif /* Base File Name */

#ifdef __TIMESTAMP__
    #define kCXTimestamp            __TIMESTAMP__
#else /* !defined(__TIMESTAMP__) */
    #define __TIMESTAMP__           "Unknown"
#endif /* Current file timestamp */

#ifdef __clang_version__
    #define kCXCompilerVersion      __clang_version__
#elif defined(__VERSION__)
    #define kCXCompilerVersion      __VERSION__
#else /* !defined(__clang_version) */
    #define kCXCompilerVersion      "Unknown"
#endif /* Clang Version */

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Set hard requirements for building CX //
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= //
// Compiler MUST be GCC-like (GCC or clang)

#if !kCXCompilerGCCLike
    #error Unsupported Compiler!
#endif /* Compiler */

// Host System MUST be *NIX or Corona
#if kCXTargetOSWindows
    #error Unsupported Host System!
#endif /* Host System */

// Architecture MUST be 64-Bit
#if kCXABI32Bit
    #error CPU Bus Size Too Small!
#endif /* CPU Bus Size */

// Architecture MUST be little endian
#if kCXBigEndian
    #error Unsupported Endianess
#endif /* Endianness */

// Language MUST be GNU C11, GNU C++14, or Objective-C 2.0
#if (kCXLanguageC && !kCXLanguageC11) || (kCXLanguageCXX && !kCXLanguageCXX14) || (kCXLanguageObjC && !kCXObjC2) || !kCXLanguageGNUC
    #error Unsupported Language Version!
#endif /* Language Version */

// Required Extensions
#if !CXHasFeature(blocks)
    #error Missing 1 or more Required Extensions!
#endif /* Required Extensions */

#endif /* !defined(__CORONA_X__) */
