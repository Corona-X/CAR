#ifndef __car__
#define __car__ 1

#include "OSCAR.h"

typedef enum {
    kARSubtypeInvalid = -1,
    kARSubtype1,
    kARSubtype2,
    kARSubtypeBootX,
    kARSubtypeSystemImage
} ARSubtype;

typedef struct {
    ARSubtype subtype;
    void *address;
    OSSize size;
} ARArchive;

ARArchive *ARArchiveOpen(const OSUTF8Char *path);
ARSubtype ARDetectSubtype(const UInt8 *header);
bool ARArchiveClose(ARArchive *archive);

bool ARCreateDirectories(const OSUTF8Char *path);
bool ARCreateDirectory(const OSUTF8Char *path);

bool ARDirectoryExistsAtPath(const OSUTF8Char *path);
bool ARFileHasDataAtPath(const OSUTF8Char *path);

// car_crc32.c

UInt32 ARCRC32Init(void);
UInt32 ARCRC32Update(UInt32 checksum, void *buffer, OSSize size);
UInt32 ARCRC32Finalize(UInt32 checksum);
UInt32 ARCRC32Process(void *buffer, OSSize size);

#endif /* !defined(__car__) */
