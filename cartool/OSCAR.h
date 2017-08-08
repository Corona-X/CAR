/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* OSCAR.h - Standard Structures for the CAR Archive Format        */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* beeselmane - 27.11.2015 - 8:00 PM EST                           */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* Note: This file uses the nonstandard prefix 'CA'                */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/

#ifndef __SYSTEM_OSCAR__
#define __SYSTEM_OSCAR__ 1

#include "Corona-X.h"
#include "OSTypes.h"
#include "OSCX.h"

#if !kCXAssemblyCode

#define kCAHeaderMagic                      ((UInt8 [4]){'C', 'A', 'R', '\0'})
#define kCAHeaderVersionS1                  ((UInt8 [4]){'X', '.', 'F',  '1'})
#define kCAHeaderVersionS2                  ((UInt8 [4]){'X', '.', 'F',  '2'})
#define kCAHeaderVersionBootX               ((UInt8 [4]){'X', '.', 'F',  'b'})
#define kCAHeaderVersionSystem              ((UInt8 [4]){'X', '.', 'F',  's'})
#define kCAHeaderBootXLockAValue            0xEFFE
#define kCAHeaderBootXLockBValue            0xFEEF

#pragma mark - Subtype 1

typedef struct {
    UInt8  magic[4];
    UInt8  version[4];
    UInt64 entryTableOffset;
    UInt64 dataSectionOffset;
    UInt32 dataChecksum;
    UInt32 headerChecksum;
} CAHeaderS1;

typedef struct __attribute__((packed)) {
    UInt8  type;
    UInt8  padding[3];
    UInt64 dataOffset;
    UInt64 dataSize;
    OSUTF8Char path[0];
} CAEntryS1;

typedef OSEnum(UInt8, CAEntryType) {
    kCAEntryTypeDirectory                   = 0,
    kCAEntryTypeFile                        = 1,
    kCAEntryTypeLink                        = 2,
    kCAEntryTypeMeta                        = 0xFF
};

#pragma mark - Subtype 2

typedef struct {
    UInt8  magic[4];
    UInt8  version[4];
    UInt64 tocOffset;
    UInt64 entryTableOffset;
    UInt64 dataSectionOffset;
    UInt32 dataChecksum;
    UInt32 headerChecksum;
    UInt64 dataModification;
    UInt64 archiveSignature;
} CAHeaderS2;

typedef OSEnum(UInt8, CAEntryFlags) {
    kCAEntryFlagUTF8                        = 0x00,
    kCAEntryFlagUTF16                       = 0x01,
    kCAEntryFlagUTF32                       = 0x02,
    kCAEntryFlagMetaHasData                 = 0x80
};

typedef struct __attribute__((packed)) {
    UInt8  type;
    UInt8  flags;
    UInt16 padding;
    UInt64 dataOffset;
    UInt64 dataSize;
    OSUnicodePoint path[0];
} CAEntryS2;

typedef OSEnum(UInt8, CACompressionType) {
    kCACompressionTypeNone                  = 0,
    kCACompressionTypeLZMA                  = 1,
    kCACompressionTypeLZO                   = 2
};

typedef OSEnum(UInt8, CAEncryptionType) {
    kCAEncryptionTypeNone                   = 0,
    kCAEncryptionTypeAES                    = 1,
    kCAEncryptionTypeSerpent                = 2
};

typedef struct CADataModification {
    UInt8 encryptionCount;
    UInt8 compressionCount;
    UInt8 padding[6];
} CADataModification;

typedef struct {
    UInt64 startOffset;
    UInt64 runLength;
    UInt8 type;
    UInt8 padding[7];
} CAEncryptionInfo;

typedef struct {
    UInt64 startOffset;
    UInt64 runLength;
    UInt8 type;
    UInt8 padding[7];
} CACompressionInfo;

#pragma mark - BootX

typedef struct {
    UInt8  magic[4];   // 'C', 'A', 'R', '\0'
    UInt8  version[4]; // 'X', '.', 'F', 'b'
    UInt32 bootID;
    UInt16 processorType;
    UInt16 lockA;

    UInt64 entryTableOffset;
    UInt64 dataSectionOffset;
    UInt32 dataChecksum;
    UInt32 headerChecksum;

    UInt16 kernelLoaderEntry;
    UInt16 kernelEntry;
    UInt16 bootConfigEntry;

    UInt16 systemServerEntry;
    UInt16 ioServerEntry;
    UInt16 faultServerEntry;
    UInt16 extensionDirectory;

    UInt16 lockB;
} CAHeaderBootX;

typedef OSEnum(UInt16, CAProcessorType) {
    kCAProcessorTypeX86                    = kCXProcessorTypeX86,
    kCAProcessorTypeX86_64                 = kCXProcessorTypeX86_64,
    kCAProcessorTypeARMv7                  = kCXProcessorTypeARMv7,
    kCAProcessorTypeARMv8                  = kCXProcessortypeARMv8
};

#pragma mark - System Image

typedef struct {
    UInt8  magic[4];   // 'C', 'A', 'R', '\0'
    UInt8  version[4]; // 'X', '.', 'F', 's'

    struct CASystemVersionInternal {
        UInt64 type:2;         // OS Type
        UInt64 majorVersion:7; // Major Version Number
        UInt64 revision:5;     // OS Revision (English Letter)
        UInt64 buildType:2;    // OS Build Type
        UInt64 buildID:48;     // System Build ID
    } systemVersion;

    UInt64 tocOffset;
    UInt64 entryTableOffset;
    UInt64 dataSectionOffset;
    UInt32 dataChecksum;
    UInt32 headerChecksum;
    UInt64 dataModification;
    UInt64 archiveSignature;
} CAHeaderSystemImage;

typedef struct CASystemVersionInternal CASystemVersionInternal;

typedef OSEnum(UInt16, CASystemType) {
    kCASystemTypeCoronaX                   = 0,
    kCASystemTypeCorOS                     = 1,
    kCASystemTypeXMobile                   = 2,
    kCASystemTypeCMobile                   = 3
};

typedef OSEnum(UInt16, CASystemBuildType) {
    kCASystemBuildTypeDebug                = 0,
    kCASystemBuildTypeDevelopment          = 1,
    kCASystemBuildTypeRelease              = 2,
    kCASystemBuildTypeStable               = 3
};

typedef struct __attribute__((packed)) {
    UInt8 type;
    UInt8 flags;
    UInt16 specialFlags;

    UInt32 parentEntry; // Parent directory
    UInt32 nextEntry;   // Next Entry in parent
    UInt32 entryCount;  // Number of entries
    UInt32 firstEntry;  // First entry

    OSUnicodePoint path[0];
} CASystemDirectoryEntry;

typedef OSEnum(UInt16, CASystemDirectoryFlags) {
    kCASystemDirectoryFlagCanEnumerate     = 0x8000,
    kCASystemDirectoryFlagAllowsExecute    = 0x4000,
};

typedef struct __attribute__((packed)) {
    UInt8 type;
    UInt8 flags;
    UInt16 specialFlags;
    
    UInt32 parentEntry; // Parent directory
    UInt32 nextEntry;   // Next Entry in parent
    UInt64 dataOffset;
    UInt64 dataSize;
    
    OSUnicodePoint path[0];
} CASystemFileEntry;

typedef OSEnum(UInt16, CASystemFileFlags) {
    kCASystemFileFlagCanRead               = 0x0001,
    kCASystemFileFlagCanUserRead           = 0x0002,
    kCASystemFileCanExecute                = 0x0004,
    kCASystemFileCanUserExecute            = 0x0010,
};

#endif /* !kCXAssemblyCode */

#endif /* !defined(__SYSTEM_OSCAR__) */
