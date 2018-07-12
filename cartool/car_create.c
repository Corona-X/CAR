#include <sys/syslimits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>

#include "car_create.h"

#define OSAlignUpward(p, s) (((p) + ((s) - 1)) & (~((s) - 1)))
#define ARAlignEntry(addr)  (((addr) - 5) & (~7)) + 12;
#define kARBlockSize        512

typedef struct {
    struct ARDirectoryEntry {
        struct ARDirectoryEntry *previous;
        struct ARDirectoryEntry *next;

        struct ARDirectoryEntry *parent;
        struct ARDirectoryEntry *nextEntry;
        struct ARDirectoryEntry *firstChild;
        UInt32 children;
        UInt32 entryID;

        OSUTF8Char *path;
        UInt64 size;
        UInt8 type;
    } *head, *tail;

    OSCount entryCount;
    OSSize fullSize;
    OSSize nameSkip;
} ARDirectoryStructure;

typedef struct ARDirectoryEntry ARDirectoryEntry;

#pragma mark - Create Functions I

static bool ARCreatePretest(const OSUTF8Char *rootDirectory, const OSUTF8Char *archive)
{
    if (!ARDirectoryExistsAtPath(rootDirectory))
    {
        fprintf(stderr, "Error: Root directory for archive does not exist!\n");
        return false;
    }

    if (ARFileHasDataAtPath(archive))
    {
        fprintf(stderr, "Error: Archive exists at path '%s' and is not empty!\n", archive);
        return false;
    }

    return true;
}

static int ARCreateOpenArchive(const OSUTF8Char *archive)
{
    int fd = open((char *)archive, O_RDWR | O_CREAT);

    if (fd == -1)
    {
        fprintf(stderr, "Error: Could not open or create archive file!\n");
        return -1;
    }

    if (chmod((char *)archive, 0644))
    {
        fprintf(stderr, "Error: Could not set permissions on archive!\n");
        return -1;
    }

    return fd;
}

static bool ARCreateSeekInArchive(int fd, OSOffset offset)
{
    if (lseek(fd, offset, SEEK_SET) != offset)
    {
        fprintf(stderr, "Error: Could not seek in archive!\n");
        return false;
    }

    return true;
}

static void *ARCreateMapArchive(int fd, OSSize size)
{
    if (ftruncate(fd, size))
    {
        fprintf(stderr, "Error: Could not expand archive to %lu bytes!\n", size);
        return MAP_FAILED;
    }

    void *address = mmap(kOSNullPointer, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (address == MAP_FAILED)
    {
        fprintf(stderr, "Error: Could not map archive into memory!\n");
        return MAP_FAILED;
    }

    return address;
}


static bool ARCreateUnmapArchive(void *archive, OSSize size)
{
    if (munmap(archive, size))
    {
        fprintf(stderr, "Error: Could not unmap archive!\n");
        return false;
    }

    return true;
}

static bool ARCreateCloseArchive(int fd)
{
    if (close(fd))
    {
        fprintf(stderr, "Error: Could not close archive!\n");
        return false;
    }

    return true;
}

#pragma mark - Write Helpers

static bool ARCreateWriteToCEntry(int fd, OSOffset offset, OSOffset entry)
{
    if (pwrite(fd, &entry, sizeof(UInt64), offset) != sizeof(UInt64))
    {
        fprintf(stderr, "Error: Could not write ToC entry!\n");
        return false;
    }

    return true;
}

static bool ARCreateWriteEntryStandard(int fd, void *entry)
{
    if (write(fd, entry, sizeof(CAEntryS1)) != sizeof(CAEntryS1))
    {
        fprintf(stderr, "Error: Could not write entry!\n");
        return false;
    }

    return true;
}

static OSOffset ARCreateWriteAlignedPath(int fd, const OSUTF8Char *path, OSOffset currentOffset)
{
    OSSize pathSize = strlen((char *)path) + 1;
    currentOffset += pathSize;
    const UInt64 zero = 0;

    if (write(fd, path, pathSize) != pathSize)
    {
        fprintf(stderr, "Error: Could not write path '%s' into archive\n", path);
        return -1;
    }

    OSCount zeros = ((currentOffset - 1) & (~7)) + 8;
    zeros -= currentOffset;
    currentOffset += zeros;

    if (zeros && (write(fd, &zero, zeros) != zeros))
    {
        fprintf(stderr, "Error: Could not align path '%s'\n", path);
        return -1;
    }

    return currentOffset;
}

static bool ARCreateWriteFile(void *destination, const OSUTF8Char *file, OSSize size)
{
    int fd = open((char *)file, O_RDONLY);

    if (fd == -1)
    {
        fprintf(stderr, "Error: Could not open file '%s'!\n", file);
        return false;
    }

    if (read(fd, destination, size) != size)
    {
        fprintf(stderr, "Error: Could not read proper number of bytes from '%s' (has it been modified?)\n", file);
        return false;
    }

    if (close(fd))
    {
        fprintf(stderr, "Error: Could not close file '%s'!", file);
        return false;
    }

    return true;
}

static bool ARCreateWriteSymlink(void *destination, const OSUTF8Char *link, OSSize size)
{
    ssize_t linkSize = size;

    if (readlink((char *)link, destination, size) != linkSize)
    {
        fprintf(stderr, "Error: Could not read symlink at '%s' (has it been modified?)\n", link);
        return false;
    }

    return true;
}

#pragma mark - Utility Functions

static ARDirectoryStructure *ARDirectoryStructureCreate(const OSUTF8Char *rootDirectory)
{
    ARDirectoryStructure *directory = malloc(sizeof(ARDirectoryStructure));
    ARDirectoryEntry *rootEntry = malloc(sizeof(ARDirectoryEntry));
    OSSize nameSkip = strlen((char *)rootDirectory);

    if (!directory || !rootEntry)
    {
        if (rootEntry) free(rootEntry);
        if (directory) free(directory);

        fprintf(stderr, "Error: Out of memory!\n");
        return kOSNullPointer;
    }

    memset(directory, 0, sizeof(ARDirectoryStructure));
    directory->head = directory->tail = rootEntry;
    directory->nameSkip = nameSkip;
    directory->entryCount = 1;

    memset(rootEntry, 0, sizeof(ARDirectoryEntry));
    rootEntry->next = rootEntry->previous = 0;
    rootEntry->type = kCAEntryTypeDirectory;
    rootEntry->path = malloc(nameSkip + 2);
    rootEntry->size = 0;

    if (!rootEntry->path)
    {
        free(rootEntry);
        free(directory);

        fprintf(stderr, "Error: Out of memory!\n");
    }

    strcpy((char *)rootEntry->path, (char *)rootDirectory);
    rootEntry->path[nameSkip + 1] = 0;
    rootEntry->path[nameSkip] = 0;

    return directory;
}

static void ARDirectoryStructureFree(ARDirectoryStructure *directory)
{
    ARDirectoryEntry *entry = directory->head;
    ARDirectoryEntry *prev;

    while (entry)
    {
        free(entry->path);

        prev = entry;
        entry = entry->next;

        free(prev);
    }

    free(directory);
}

static bool ARPromptContinue(void)
{
    fprintf(stderr, "(C)ontinue or (Q)uit? ");
    char buffer[256];

    if (fgets(buffer, 256, stdin) != buffer)
    {
        fprintf(stderr, "Input error!\n");

        return false;
    }

    char *newline = memchr(buffer, '\n', strlen(buffer));
    *newline = 0;

    if (!buffer[1])
    {
        if (tolower(buffer[0]) == 'c') {
            return true;
        } else if (tolower(buffer[0]) == 'q') {
             return false;
        }
    }

    fprintf(stderr, "Unknown option '%s'\n", buffer);
    return false;
}

#pragma mark - Directory Enumeration

static bool AREnumerateDirectory(ARDirectoryStructure *directory, bool system, bool verbose)
{
    DIR *dir = opendir((const char *)directory->tail->path);
    ARDirectoryEntry *iteration = directory->tail;
    ARDirectoryEntry *lastEntry = kOSNullPointer;
    struct dirent *entry;
    char type;

    if (!dir)
    {
        fprintf(stderr, "Error: Unknown directory read or access error for directory '%s'!\n", directory->tail->path);

        if (ARPromptContinue()) {
            directory->tail = directory->tail->previous;
            free(directory->tail->next->path);
            free(directory->tail->next);

            if (system)
            {
                if (iteration->parent->firstChild == iteration)
                    iteration->parent->firstChild = kOSNullPointer;

                iteration->children--;
            }

            directory->tail->next = kOSNullPointer;
            return true;
        } else {
            return false;
        }
    }

    while ((entry = readdir(dir)))
    {
        if (!strncmp(entry->d_name, ".DS_Store", entry->d_namlen)) continue;
        if (!strncmp(entry->d_name, "..", entry->d_namlen)) continue;
        if (!strncmp(entry->d_name, ".", entry->d_namlen)) continue;

        OSUTF8Char *entryPath; asprintf((char **)&entryPath, "%s/%s", iteration->path, entry->d_name);
        ARDirectoryEntry *entryData = malloc(sizeof(ARDirectoryEntry));
        struct stat stats;

        if (!entryPath || !entryData)
        {
            if (entryPath) free(entryPath);
            if (entryData) free(entryData);

            fprintf(stderr, "Error: Out of memory!\n");
            return false;
        }

        entryData->previous = directory->tail;
        directory->tail->next = entryData;
        entryData->next = kOSNullPointer;

        entryData->path = entryPath;
        directory->entryCount++;

        if (lstat((char *)entryData->path, &stats))
        {
            fprintf(stderr, "Error: Permission denied at path '%s'!\n", entryPath);
            return false;
        }

        if (S_ISLNK(stats.st_mode)) {
            OSUTF8Char link[PATH_MAX + 1];
            ssize_t length;

            if ((length = readlink((char *)entryPath, (char *)link, PATH_MAX + 1)) == -1)
            {
                fprintf(stderr, "Error: Couldn't read the contents of the symlink at '%s'!\n", entryPath);
                return false;
            }

            entryData->type = kCAEntryTypeLink;
            entryData->size = length;
            type = 'L';
        } else if (S_ISDIR(stats.st_mode)) {
            entryData->type = kCAEntryTypeDirectory;
            entryData->size = 0;

            if (access((char *)entryData->path, R_OK | X_OK))
            {
                fprintf(stderr, "Error: Can't access directory '%s'!\n", entryPath);
                return false;
            }

            type = 'D';
        } else if (S_ISREG(stats.st_mode)) {
            entryData->type = kCAEntryTypeFile;
            entryData->size = stats.st_size;

            if (access((char *)entryData->path, R_OK))
            {
                fprintf(stderr, "Error: Can't access file '%s'!\n", entryPath);
                return false;
            }

            type = 'F';
        } else {
            directory->tail->next = kOSNullPointer;
            directory->entryCount--;

            free(entryData);
            free(entryPath);

            type = 'S';
        }

        if (verbose) fprintf(stdout, "%c %s\n", type, entryPath);

        if (system)
        {
            entryData->parent = iteration;
            iteration->children++;

            if (!iteration->firstChild)
                iteration->firstChild = entryData;

            if (lastEntry)
                lastEntry->nextEntry = entryData;

            if (type == 'D')
            {
                entryData->firstChild = kOSNullPointer;
                entryData->children = 0;
            }

            entryData->entryID = directory->tail->entryID + 1;
            entryData->nextEntry = kOSNullPointer;
            lastEntry = entryData;
        }

        if (type != 'S')
        {
            directory->fullSize += entryData->size;
            directory->tail = entryData;
        }

        if (type == 'D')
            if (!AREnumerateDirectory(directory, system, verbose)) return false;
    }

    closedir(dir);
    return true;
}

#pragma mark - Create Functions II

static OSOffset ARCreateWriteToCAndEntries(ARSubtype subtype, ARDirectoryStructure *directory, int fd, OSOffset tocOffset, bool verbose)
{
    const OSSize directoryEntrySize = sizeof(CAEntryS2) - (2 * sizeof(UInt64));
    ARDirectoryEntry *entry = directory->head;
    OSOffset entryOffset = 0;
    OSOffset dataOffset = 0;
    CAEntryS2 fileEntry;

    memset(&fileEntry, 0, sizeof(CAEntryS2));

    while (entry)
    {
        if (!ARCreateWriteToCEntry(fd, tocOffset, entryOffset))
        {
            ARDirectoryStructureFree(directory);
            ARCreateCloseArchive(fd);

            return -1;
        }

        tocOffset += sizeof(UInt64);
        fileEntry.type = entry->type;

        switch (fileEntry.type)
        {
            case kCAEntryTypeDirectory: {
                fileEntry.dataOffset = 0;
                fileEntry.dataSize = 0;
            } break;
            case kCAEntryTypeLink:
            case kCAEntryTypeFile: {
                fileEntry.dataOffset = dataOffset;
                fileEntry.dataSize = entry->size;

                dataOffset += entry->size;
            } break;
        }

        if (subtype != kARSubtype1 && fileEntry.type == kCAEntryTypeDirectory) {
            if (write(fd, &fileEntry, directoryEntrySize) != directoryEntrySize)
            {
                ARDirectoryStructureFree(directory);
                ARCreateCloseArchive(fd);

                return -1;
            }

            entryOffset += directoryEntrySize;
        } else {
            if (!ARCreateWriteEntryStandard(fd, &fileEntry))
            {
                ARDirectoryStructureFree(directory);
                ARCreateCloseArchive(fd);

                return -1;
            }

            entryOffset += sizeof(CAEntryS2);
        }

        OSUTF8Char *path = entry->path + directory->nameSkip;
        entryOffset = ARCreateWriteAlignedPath(fd, path, entryOffset);

        if (entryOffset == -1)
        {
            ARDirectoryStructureFree(directory);
            ARCreateCloseArchive(fd);

            return -1;
        }

        if (verbose) fprintf(stdout, "E %s\n", path);
        entry = entry->next;
    }

    return entryOffset;
}

static OSOffset ARCreateWriteToCAndEntriesSystemImage(ARSubtype subtype, ARDirectoryStructure *directory, int fd, OSOffset tocOffset, bool verbose)
{
    ARDirectoryEntry *entry = directory->head;
    OSOffset entryOffset = 0;
    OSOffset dataOffset = 0;

    while (entry)
    {
        if (!ARCreateWriteToCEntry(fd, tocOffset, entryOffset))
        {
            ARDirectoryStructureFree(directory);
            ARCreateCloseArchive(fd);

            return -1;
        }

        tocOffset += sizeof(UInt64);

        switch (entry->type)
        {
            case kCAEntryTypeDirectory: {
                CASystemDirectoryEntry archiveEntry;

                archiveEntry.type = kCAEntryTypeDirectory;
                archiveEntry.specialFlags = 0xDD;

                if (entry->parent) archiveEntry.parentEntry = entry->parent->entryID;
                else               archiveEntry.parentEntry = 0;

                if (entry->nextEntry) archiveEntry.nextEntry = entry->nextEntry->entryID;
                else                  archiveEntry.nextEntry = 0;

                if (archiveEntry.firstEntry) archiveEntry.firstEntry = entry->firstChild->entryID;
                else                         archiveEntry.firstEntry = 0;

                archiveEntry.entryCount = entry->children;

                if (write(fd, &archiveEntry, sizeof(CASystemDirectoryEntry)) != sizeof(CASystemDirectoryEntry))
                {
                    fprintf(stderr, "Error: Could not write directory entry!\n");

                    ARDirectoryStructureFree(directory);
                    ARCreateCloseArchive(fd);

                    return -1;
                }

                entryOffset += sizeof(CASystemDirectoryEntry);
            } break;
            case kCAEntryTypeLink:
            case kCAEntryTypeFile: {
                CASystemFileEntry archiveEntry;

                archiveEntry.type = entry->type;
                archiveEntry.specialFlags = 0xFF;

                if (entry->parent) archiveEntry.parentEntry = entry->parent->entryID;
                else               archiveEntry.parentEntry = 0;

                if (entry->nextEntry) archiveEntry.nextEntry = entry->nextEntry->entryID;
                else                  archiveEntry.nextEntry = 0;

                archiveEntry.dataOffset = dataOffset;
                archiveEntry.dataSize = entry->size;

                if (write(fd, &archiveEntry, sizeof(CASystemFileEntry)) != sizeof(CASystemFileEntry))
                {
                    fprintf(stderr, "Error: Could not write file entry!\n");

                    ARDirectoryStructureFree(directory);
                    ARCreateCloseArchive(fd);

                    return -1;
                }

                entryOffset += sizeof(CASystemFileEntry);
                dataOffset += entry->size;
            } break;
        }

        OSUTF8Char *path = entry->path + directory->nameSkip;
        entryOffset = ARCreateWriteAlignedPath(fd, path, entryOffset);

        if (entryOffset == -1)
        {
            ARDirectoryStructureFree(directory);
            ARCreateCloseArchive(fd);

            return -1;
        }

        if (verbose) fprintf(stdout, "E %s\n", path);
        entry = entry->next;
    }

    return entryOffset;
}

static bool CACreateWriteDataSection(ARDirectoryStructure *directory, void *file, OSSize archiveSize, OSOffset dataOffset, bool verbose)
{
    ARDirectoryEntry *entry = directory->head;

    while (entry)
    {
        bool failed = false;

        switch (entry->type)
        {
            case kCAEntryTypeLink: {
                failed = !ARCreateWriteSymlink(file + dataOffset, entry->path, entry->size);
            } break;
            case kCAEntryTypeFile: {
                failed = !ARCreateWriteFile(file + dataOffset, entry->path, entry->size);
            } break;
        }

        if (failed)
        {
            ARCreateUnmapArchive(file, archiveSize);
            ARDirectoryStructureFree(directory);

            return false;
        }

        if (verbose && (entry->type != kCAEntryTypeDirectory))
            fprintf(stdout, "W %s\n", entry->path);

        dataOffset += entry->size;
        entry = entry->next;
    }

    return true;
}

#pragma mark - Creation Functions

typedef struct {
    void *address;
    OSSize archiveSize;

    OSOffset entryOffset;
    OSOffset dataOffset;
} ARCreateInfo;

ARCreateInfo *ARCreateArchive(ARSubtype subtype, const OSUTF8Char *rootDirectory, const OSUTF8Char *archive, OSOffset tocOffset, bool verbose)
{
    if (!ARCreatePretest(rootDirectory, archive))
        return kOSNullPointer;

    ARDirectoryStructure *directory = ARDirectoryStructureCreate(rootDirectory);
    if (!directory) return kOSNullPointer;

    if (verbose) fprintf(stdout, "D /\n");
    bool haveStructure = AREnumerateDirectory(directory, (subtype == kARSubtypeSystemImage), verbose);
    directory->head->path[directory->nameSkip] = '/';

    if (!haveStructure)
    {
        ARDirectoryStructureFree(directory);
        return kOSNullPointer;
    }

    int fd = ARCreateOpenArchive(archive);

    if (fd == -1)
    {
        ARDirectoryStructureFree(directory);
        return kOSNullPointer;
    }

    OSOffset entryTableOffset = tocOffset + (sizeof(UInt64) * directory->entryCount);
    if (subtype == kARSubtypeSystemImage) entryTableOffset = OSAlignUpward(entryTableOffset, kARBlockSize);
    entryTableOffset += sizeof(UInt32); // Entry Table is offset by 4 bytes

    if (!ARCreateSeekInArchive(fd, entryTableOffset))
    {
        ARDirectoryStructureFree(directory);
        ARCreateCloseArchive(fd);

        return kOSNullPointer;
    }

    OSOffset finalEntryOffset;

    if (subtype == kARSubtypeSystemImage) finalEntryOffset = ARCreateWriteToCAndEntriesSystemImage(subtype, directory, fd, tocOffset, verbose);
    else finalEntryOffset = ARCreateWriteToCAndEntries(subtype, directory, fd, tocOffset, verbose);

    if (finalEntryOffset == -1)
        return kOSNullPointer;

    OSOffset dataOffset = entryTableOffset + finalEntryOffset;

    if (subtype == kARSubtypeSystemImage) dataOffset = OSAlignUpward(dataOffset, kARBlockSize);
    else dataOffset = OSAlignUpward(dataOffset, 8);

    if (!ARCreateSeekInArchive(fd, dataOffset))
    {
        ARDirectoryStructureFree(directory);
        ARCreateCloseArchive(fd);
        
        return kOSNullPointer;
    }

    OSSize archiveSize = dataOffset + directory->fullSize;
    OSUTF8Char *file = ARCreateMapArchive(fd, archiveSize);
    OSOffset dataSectionOffset = dataOffset;

    if (file == MAP_FAILED)
    {
        ARDirectoryStructureFree(directory);
        ARCreateCloseArchive(fd);

        return kOSNullPointer;
    }

    if (!ARCreateCloseArchive(fd))
    {
        ARCreateUnmapArchive(file, archiveSize);
        ARDirectoryStructureFree(directory);

        return kOSNullPointer;
    }

    if (!CACreateWriteDataSection(directory, file, archiveSize, dataOffset, verbose))
        return kOSNullPointer;

    ARDirectoryStructureFree(directory);
    ARCreateInfo *stats = malloc(sizeof(ARCreateInfo));

    if (!stats)
    {
        fprintf(stderr, "Error: Out of memory!\n");
        ARCreateUnmapArchive(file, archiveSize);

        return false;
    }

    stats->archiveSize = archiveSize;
    stats->address = file;

    stats->entryOffset = entryTableOffset;
    stats->dataOffset = dataSectionOffset;

    return stats;
}

bool ARCreateSubtype1(const OSUTF8Char *rootDirectory, const OSUTF8Char *archive, bool verbose)
{
    ARCreateInfo *stats = ARCreateArchive(kARSubtype1, rootDirectory, archive, sizeof(CAHeaderS1), verbose);
    if (!stats) return false;

    CAHeaderS1 *header = stats->address;
    memcpy(&header->magic, kCAHeaderMagic, 4);
    memcpy(&header->version, kCAHeaderVersionS1, 4);

    header->dataSectionOffset = stats->dataOffset;
    header->entryTableOffset = stats->entryOffset;

    if (verbose) fprintf(stdout, "Generating checksums...\n");
    header->dataChecksum = ARCRC32Process(stats->address + sizeof(CAHeaderS1), stats->archiveSize - sizeof(CAHeaderS1));
    header->headerChecksum = ARCRC32Process(header, sizeof(CAHeaderS1) - (2 * sizeof(UInt32)));

    if (verbose) printf("header: 0x%08X\ndata: 0x%08X\n", header->headerChecksum, header->dataChecksum);
    bool failed = ARCreateUnmapArchive(stats->address, stats->archiveSize);
    free(stats);

    return !failed;
}

bool ARCreateSubtype2(const OSUTF8Char *rootDirectory, const OSUTF8Char *archive, bool verbose, ARCreateDataModifiers *modifiers)
{
    ARCreateInfo *stats = ARCreateArchive(kARSubtype2, rootDirectory, archive, sizeof(CAHeaderS2) + sizeof(CADataModification), verbose);
    if (!stats) return false;

    CAHeaderS2 *header = stats->address;
    memcpy(&header->magic, kCAHeaderMagic, 4);
    memcpy(&header->version, kCAHeaderVersionS2, 4);

    header->tocOffset = sizeof(CAHeaderS2) + sizeof(CADataModification);
    header->dataModification = sizeof(CAHeaderS2);
    header->dataSectionOffset = stats->dataOffset;
    header->entryTableOffset = stats->entryOffset;

    if (verbose) fprintf(stdout, "Generating checksums...\n");
    header->dataChecksum = ARCRC32Process(stats->address + sizeof(CAHeaderS2), stats->archiveSize - sizeof(CAHeaderS2));

    header->headerChecksum = ARCRC32Init();
    header->headerChecksum = ARCRC32Update(header->headerChecksum, header, sizeof(CAHeaderS2) - (3 * sizeof(UInt64)));
    header->headerChecksum = ARCRC32Update(header->headerChecksum, header + (sizeof(CAHeaderS2) - (3 * sizeof(UInt64))), 2 * sizeof(UInt64));
    header->headerChecksum = ARCRC32Finalize(header->headerChecksum);

    if (verbose) printf("header: 0x%08X\ndata: 0x%08X\n", header->headerChecksum, header->dataChecksum);
    bool failed = ARCreateUnmapArchive(stats->address, stats->archiveSize);
    free(stats);

    return !failed;
}

bool ARCreateBootX(const OSUTF8Char *rootDirectory, const OSUTF8Char *archive, bool verbose, ARCreateDataModifiers *modifiers, UInt16 architecture, UInt32 bootID, const OSUTF8Char *kernelLoaderPath, const OSUTF8Char *kernelPath, const OSUTF8Char *bootConfigPath)
{
    ARCreateInfo *stats = ARCreateArchive(kARSubtype2, rootDirectory, archive, sizeof(CAHeaderS2) + sizeof(CADataModification), verbose);
    if (!stats) return false;

    CAHeaderBootX *header = stats->address;
    memcpy(&header->magic, kCAHeaderMagic, 4);
    memcpy(&header->version, kCAHeaderVersionBootX, 4);

    header->dataSectionOffset = stats->dataOffset;
    header->entryTableOffset = stats->entryOffset;

    header->processorType = architecture;
    header->bootID = bootID;

    OSOffset *toc = stats->address + sizeof(CAHeaderBootX) + sizeof(CADataModification);
    OSOffset *final = stats->address + header->entryTableOffset - sizeof(UInt32);
    OSSize skip = strlen((char *)rootDirectory);
    OSOffset *base = toc;

    while (toc < final)
    {
        CAEntryS2 *entry = stats->address + header->entryTableOffset + (*toc);

        if (entry->type == kCAEntryTypeFile)
        {
            if (!header->kernelLoaderEntry && !strcmp((char *)entry->path, (char *)(kernelLoaderPath + skip))) {
                header->kernelLoaderEntry = toc - base;

                if (verbose) fprintf(stdout, "Kernel Loader Entry: %hu\n", header->kernelLoaderEntry);
            } else if (!header->kernelEntry && !strcmp((char *)entry->path, (char *)(kernelPath + skip))) {
                header->kernelEntry = toc - base;

                if (verbose) fprintf(stdout, "Kernel Entry: %hu\n", header->kernelEntry);
            } else if (!header->bootConfigEntry && !strcmp((char *)entry->path, (char *)(bootConfigPath + skip))) {
                header->bootConfigEntry = toc - base;

                if (verbose) fprintf(stdout, "Boot Config Entry: %hu\n", header->bootConfigEntry);
            }
        }

        toc++;
    }

    header->lockA = kCAHeaderBootXLockAValue;
    header->lockB = kCAHeaderBootXLockBValue;

    if (verbose) fprintf(stdout, "Generating checksums...\n");
    header->dataChecksum = ARCRC32Process(stats->address + sizeof(CAHeaderBootX), stats->archiveSize - sizeof(CAHeaderBootX));

    header->headerChecksum = ARCRC32Init();
    header->headerChecksum = ARCRC32Update(header->headerChecksum, header, sizeof(CAHeaderBootX) - (3 * sizeof(UInt64)));
    header->headerChecksum = ARCRC32Update(header->headerChecksum, header + (sizeof(CAHeaderBootX) - (3 * sizeof(UInt64))), 2 * sizeof(UInt64));
    header->headerChecksum = ARCRC32Finalize(header->headerChecksum);

    if (verbose) printf("header: 0x%08X\ndata: 0x%08X\n", header->headerChecksum, header->dataChecksum);
    bool failed = ARCreateUnmapArchive(stats->address, stats->archiveSize);
    free(stats);

    return !failed;
}

bool ARCreateSystemImage(const OSUTF8Char *rootDirectory, const OSUTF8Char *archive, bool verbose, ARCreateDataModifiers *modifiers, CASystemVersionInternal *systemVersion, const OSUTF8Char *partitionInfoPath, const OSUTF8Char *bootArchivePath)
{
    ARCreateInfo *stats = ARCreateArchive(kARSubtypeSystemImage, rootDirectory, archive, kARBlockSize * 2, verbose);
    if (!stats) return false;

    CAHeaderSystemImage *header = stats->address;
    memcpy(&header->magic, kCAHeaderMagic, 4);
    memcpy(&header->version, kCAHeaderVersionSystem, 4);
    memcpy(&header->systemVersion, systemVersion, sizeof(CASystemVersionInternal));

    header->tocOffset = 2 * kARBlockSize;
    header->dataSectionOffset = stats->dataOffset;
    header->entryTableOffset = stats->entryOffset;
    header->dataModification = kARBlockSize;

    if (bootArchivePath) {
        OSOffset *final = stats->address + header->entryTableOffset - sizeof(UInt32);
        OSOffset *toc = stats->address + header->tocOffset;
        OSSize skip = strlen((char *)rootDirectory);
        OSOffset *base = toc;

        while (toc < final)
        {
            CAEntryS2 *baseEntry = stats->address + header->entryTableOffset + (*toc);

            if (baseEntry->type == kCAEntryTypeFile)
            {
                CASystemFileEntry *entry = (CASystemFileEntry *)baseEntry;

                if (!strcmp((char *)entry->path, (char *)(bootArchivePath + skip)))
                {
                    header->bootEntry = toc - base;

                    if (verbose)
                        fprintf(stdout, "Kernel Loader Entry: %lu\n", header->bootEntry);

                    break;
                }
            }

            toc++;
        }
    } else {
        // This means 'none'
        header->bootEntry = ~((UInt64)0);
    }

    if (verbose) fprintf(stdout, "Generating checksums...\n");
    header->dataChecksum = ARCRC32Process(stats->address + sizeof(CAHeaderSystemImage), stats->archiveSize - sizeof(CAHeaderSystemImage));

    // Offset into the header for the checksum (ignored during calculation)
    UInt32 headerChecksumOffset = sizeof(CAHeaderSystemImage) - (3 * sizeof(UInt64));

    header->headerChecksum = ARCRC32Init();
    header->headerChecksum = ARCRC32Update(header->headerChecksum, header, headerChecksumOffset);
    header->headerChecksum = ARCRC32Update(header->headerChecksum, header + headerChecksumOffset, kARBlockSize - (headerChecksumOffset + sizeof(UInt32)));
    header->headerChecksum = ARCRC32Finalize(header->headerChecksum);

    if (verbose) printf("header: 0x%08X\ndata: 0x%08X\n", header->headerChecksum, header->dataChecksum);
    bool failed = ARCreateUnmapArchive(stats->address, stats->archiveSize);
    free(stats);

    return !failed;
}
