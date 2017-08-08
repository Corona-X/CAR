#include <sys/syslimits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "car.h"

ARArchive *ARArchiveOpen(const OSUTF8Char *path)
{
    struct stat stats;

    if (stat((char *)path, &stats))
    {
        fprintf(stderr, "Error: Could not get status of archive '%s'\n", path);
        return false;
    }

    int fd = open((char *)path, O_RDONLY);
    
    if (fd == -1)
    {
        fprintf(stderr, "Error: Could not open archive '%s'!\n", path);
        return kOSNullPointer;
    }

    void *address = mmap(kOSNullPointer, stats.st_size, PROT_READ, MAP_SHARED, fd, 0);

    if (address == MAP_FAILED)
    {
        fprintf(stderr, "Error: Could not map archive '%s'!\n", path);

        if (close(fd))
            fprintf(stderr, "Error: Could not close archive '%s'!\n", path);

        return kOSNullPointer;
    }

    if (close(fd))
    {
        fprintf(stderr, "Error: Could not close archive '%s'!\n", path);

        if (munmap(address, stats.st_size))
            fprintf(stderr, "Error: Could not unmap archive '%s'!\n", path);

        return kOSNullPointer;
    }

    ARSubtype subtype = ARDetectSubtype(address);

    if (subtype == kARSubtypeInvalid)
    {
        fprintf(stderr, "Error: Archive '%s' is of invalid format!\n", path);

        if (munmap(address, stats.st_size))
            fprintf(stderr, "Error: Could not unmap archive '%s'\n", path);

        return kOSNullPointer;
    }

    ARArchive *archive = malloc(sizeof(ARArchive));

    if (!archive)
    {
        fprintf(stderr, "Error: Out of memory!\n");

        if (munmap(address, stats.st_size))
            fprintf(stderr, "Error: Could not unmap archive '%s'!\n", path);

        return kOSNullPointer;
    }

    archive->size = stats.st_size;
    archive->subtype = subtype;
    archive->address = address;

    return archive;
}

ARSubtype ARDetectSubtype(const UInt8 *buffer)
{
    CAHeaderS1 *header = (CAHeaderS1 *)buffer;

    if (memcmp(header->magic, kCAHeaderMagic, 4))
        return kARSubtypeInvalid;

    if (!memcmp(header->version, kCAHeaderVersionS1, 4)) {
        return kARSubtype1;
    } else if (!memcmp(header->version, kCAHeaderVersionS2, 4)) {
        return kARSubtype2;
    } else if (!memcmp(header->version, kCAHeaderVersionBootX, 4)) {
        return kARSubtypeBootX;
    } else if (!memcmp(header->version, kCAHeaderVersionSystem, 4)) {
        return kARSubtypeSystemImage;
    } else {
        return kARSubtypeInvalid;
    }
}

bool ARArchiveClose(ARArchive *archive)
{
    if (munmap(archive->address, archive->size))
    {
        fprintf(stderr, "Error: Could not unmap archive!\n");
        free(archive);

        return false;
    }

    free(archive);
    return true;
}

bool ARCreateDirectories(const OSUTF8Char *path)
{
    OSUTF8Char *pointer = kOSNullPointer;
    OSUTF8Char copy[PATH_MAX + 1];

    OSSize length = snprintf((char *)copy, PATH_MAX + 1, "%s", path);

    if (copy[length - 1] == '/')
        copy[length - 1] = 0;

    for (pointer = copy + 1; (*pointer); pointer++)
    {
        if ((*pointer) == '/')
        {
            (*pointer) = 0;

            if (!ARCreateDirectory(copy))
                return false;

            (*pointer) = '/';
        }
    }

    return true;
}

bool ARCreateDirectory(const OSUTF8Char *path)
{
    if (mkdir((char *)path, S_IRWXU))
    {
        fprintf(stderr, "Error: Could not create directory '%s'\n", path);
        return false;
    }

    return true;
}

bool ARDirectoryExistsAtPath(const OSUTF8Char *path)
{
    struct stat stats;

    return !stat((char *)path, &stats);
}

bool ARFileHasDataAtPath(const OSUTF8Char *path)
{
    struct stat stats;

    if (!stat((char *)path, &stats))
        return !!stats.st_size;

    return false;
}
