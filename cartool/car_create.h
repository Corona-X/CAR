#ifndef __car_create__
#define __car_create__ 1

#include "OSCAR.h"
#include "car.h"

typedef struct {
    CACompressionType compressionType;
    CAEncryptionType encryptionType;
    bool compressToC;
    bool compressEntries;
    bool compressData;
    bool encryptArchive;
    const OSUTF8Char *signingCertificate;
} ARCreateDataModifiers;

bool ARCreateSubtype1(const OSUTF8Char *rootDirectory, const OSUTF8Char *archive, bool verbose);
bool ARCreateSubtype2(const OSUTF8Char *rootDirectory, const OSUTF8Char *archive, bool verbose, ARCreateDataModifiers *modifiers);
bool ARCreateBootX(const OSUTF8Char *rootDirectory, const OSUTF8Char *archive, bool verbose, ARCreateDataModifiers *modifiers, UInt16 architecture, UInt32 bootID, const OSUTF8Char *kernelLoaderPath, const OSUTF8Char *kernelPath, const OSUTF8Char *bootConfigPath);
bool ARCreateSystemImage(const OSUTF8Char *rootDirectory, const OSUTF8Char *archive, bool verbose, ARCreateDataModifiers *modifiers, CASystemVersionInternal *systemVersion, const OSUTF8Char *partitionInfoPath);

#endif /* !defined(__car_create__) */
