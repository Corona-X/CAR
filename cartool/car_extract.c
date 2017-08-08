#include "car_extract.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

static bool ARExtractFile(const OSUTF8Char *destination, void *address, OSSize size)
{
    if (ARFileHasDataAtPath(destination))
    {
        fprintf(stderr, "Error: File exists and is not empty at '%s'!\n", destination);
        return false;
    }

    int fd = open((char *)destination, O_CREAT | O_WRONLY);

    if (fd == -1)
    {
        fprintf(stderr, "Error: Could not open file '%s'!\n", destination);
        return false;
    }

    if (write(fd, address, size) != size)
    {
        fprintf(stderr, "Error: Could not write file '%s'!\n", destination);
        return false;
    }

    if (close(fd))
    {
        fprintf(stderr, "Error: Could not close file '%s'!\n", destination);
        return false;
    }

    return true;
}

static bool ARExtractLink(const OSUTF8Char *destination, const OSUTF8Char *link)
{
    if (symlink((char *)link, (char *)destination))
    {
        fprintf(stderr, "Could not create symlink '%s'\n", destination);
        return false;
    }

    return true;
}

static bool ARExtractArchiveSubtype1(ARArchive *archive, const OSUTF8Char *rootDirectory, bool verbose)
{
    if (!ARCreateDirectory(rootDirectory))
    {
        fprintf(stderr, "Error: Could not create root directory!\n");
        return false;
    }

    if (chdir((char *)rootDirectory))
    {
        fprintf(stderr, "Error: Could not change working directory!\n");
        return false;
    }

    CAHeaderS1 *header = archive->address;
    OSOffset *finalEntryEnd = archive->address + (header->entryTableOffset - sizeof(UInt32));
    OSOffset *toc = archive->address + sizeof(CAHeaderS1);
    char type = '?';

    while (toc < finalEntryEnd)
    {
        CAEntryS1 *entry = archive->address + header->entryTableOffset + (*toc);
        OSUTF8Char *destination = entry->path + 1;

        switch (entry->type)
        {
            case kCAEntryTypeDirectory: {
                type = 'D';

                if (!(*destination))
                    break;

                if (!ARCreateDirectory(destination))
                    return false;
            } break;
            case kCAEntryTypeFile: {
                type = 'F';

                if (!ARExtractFile(destination, archive->address + header->dataSectionOffset + entry->dataOffset, entry->dataSize))
                    return false;
            } break;
            case kCAEntryTypeLink: {
                OSUTF8Char *link = malloc(entry->dataSize + 1);
                type = 'L';

                if (!link)
                {
                    fprintf(stderr, "Error: Out of memory!\n");
                    return false;
                }

                memcpy(link, archive->address + header->dataSectionOffset + entry->dataOffset, entry->dataSize);
                link[entry->dataSize] = 0;

                bool success = ARExtractLink(destination, link);
                free(link);

                if (!success) return false;
            } break;
        }

        toc++;

        // I'd be lying if I said I know why this helps...
        // Both cases (verbose = true/false) actually go
        // about 3x faster by using __builtin_except as
        // opposed to a stanadrd if statement......
        if (__builtin_expect(verbose, false))
            fprintf(stdout, "%c %s\n", type, entry->path);
    }

    return true;
}

bool ARExtractFiles(const OSUTF8Char *archive, const OSUTF8Char *rootDirectory, ARExtractFileInfo *files, OSCount fileCount, bool verbose)
{
    return true;
}

bool ARExtractArchive(const OSUTF8Char *path, const OSUTF8Char *rootDirectory, bool verbose)
{
    ARArchive *archive = ARArchiveOpen(path);
    if (!archive) return false;

    bool success = ARExtractArchiveSubtype1(archive, rootDirectory, verbose);

    return (ARArchiveClose(archive) && success);
}
