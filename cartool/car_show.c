#include "car_show.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static bool ARShowContentsInternal(ARSubtype subtype, OSOffset *toc, UInt8 *entryTable, UInt8 *dataOffset, bool showSize, bool showLinks)
{
    OSCount entryCount = (((OSPointerValue)entryTable) - ((OSPointerValue)toc)) / sizeof(OSPointerValue);

    for (OSIndex i = 0; i < entryCount; i++)
    {
        if (i & !(*toc))
        {
            // We're done
            break;
        }

        UInt8 *entry = entryTable + (*toc);
        UInt8 entryType = *((UInt8 *)entry);
        toc++;

        CAEntryS1 linkData;
        OSUTF8Char *path;
        OSUTF8Char type;
        UInt64 size = 0;

        switch (entryType)
        {
            case kCAEntryTypeDirectory: type = 'D'; break;
            case kCAEntryTypeFile:      type = 'F'; break;
            case kCAEntryTypeLink:      type = 'L'; break;
            case kCAEntryTypeMeta:      type = 'M'; break;
            default:                    type = '?'; break;
        }

        switch (subtype)
        {
            case kARSubtype1: {
                CAEntryS1 *realEntry = (CAEntryS1 *)entry;
                path = realEntry->path;

                if (showLinks && entryType == kCAEntryTypeLink)
                    memcpy(&linkData, entry, sizeof(CAEntryS1));

                if (showSize)
                    size = realEntry->dataSize;
            } break;
            case kARSubtype2:
            case kARSubtypeBootX: {
                path = (OSUTF8Char *)(entry + sizeof(CAEntryS2));

                if (entryType == kCAEntryTypeDirectory) {
                    path -= (2 * sizeof(UInt64));
                    size = 0;
                } else if (entryType == kCAEntryTypeMeta) {
                    CAEntryS2 *realEntry = (CAEntryS2 *)entry;

                    if (!(realEntry->flags & kCAEntryFlagMetaHasData)) {
                        path -= (2 * sizeof(UInt64));
                        size = 0;
                    } else {
                        size = realEntry->dataSize;
                    }
                } else {
                    size = ((CAEntryS2 *)entry)->dataSize;
                }

                if (showLinks && entryType == kCAEntryTypeLink)
                    memcpy(&linkData, entry, sizeof(CAEntryS1));
            } break;
            case kARSubtypeSystemImage: {
                if (entryType == kCAEntryTypeDirectory) {
                    path = (OSUTF8Char *)(entry + sizeof(CASystemDirectoryEntry));
                    size = 0;
                } else {
                    size = ((CASystemFileEntry *)entry)->dataSize;
                    path = (OSUTF8Char *)(entry + sizeof(CASystemFileEntry));
                }

                if (showLinks && entryType == kCAEntryTypeLink)
                {
                    CASystemFileEntry *realEntry = (CASystemFileEntry *)entry;

                    linkData.dataOffset = realEntry->dataOffset;
                    linkData.dataSize = realEntry->dataSize;
                }
            } break;
            default: {
                path = (OSUTF8Char *)"";
                size = 0;
            } break;
        }

        fprintf(stdout, "%c %s", type, path);

        if (showSize)
            fprintf(stdout, " (%lu)", size);

        if (showLinks && entryType == kCAEntryTypeLink) {
            OSUTF8Char *link = malloc(linkData.dataSize + 1);
            memcpy(link, dataOffset + linkData.dataOffset, linkData.dataSize);
            link[linkData.dataSize] = 0;
            
            fprintf(stdout, " --> %s\n", link);
            free(link);
        } else {
            fprintf(stdout, "\n");
        }
    }

    return true;
}

static bool ARListContentsInternal(ARArchive *archive, bool showSize, bool showLinks)
{
    UInt8 *dataOffset = kOSNullPointer;
    UInt8 *entryTable = kOSNullPointer;
    OSOffset *toc = kOSNullPointer;

    bool success = true;

    #define ARCopyShared(h)                                     \
        dataOffset = archive->address + h->dataSectionOffset;   \
        entryTable = archive->address + h->entryTableOffset

    switch (archive->subtype)
    {
        case kARSubtype1: {
            CAHeaderS1 *header = archive->address;

            toc = archive->address + sizeof(CAHeaderS1);
            ARCopyShared(header);
        } break;
        case kARSubtype2: {
            CAHeaderS2 *header = archive->address;
            CADataModification *dataModification = archive->address + header->dataModification;

            toc = archive->address + header->tocOffset;
            ARCopyShared(header);

            if (dataModification->compressionCount || dataModification->encryptionCount)
                fprintf(stderr, "Warning: Archive may contain data modification!\n");
        } break;
        case kARSubtypeBootX: {
            CAHeaderBootX *header = archive->address;
            CADataModification *dataModification = archive->address + sizeof(CAHeaderBootX);
            toc = archive->address;

            if (dataModification->compressionCount || dataModification->encryptionCount) {
                fprintf(stderr, "Warning: Archive may contain data modification!\n");

                toc += (dataModification->compressionCount * sizeof(CACompressionInfo));
                toc += (dataModification->encryptionCount * sizeof(CAEncryptionInfo));
                toc += sizeof(CADataModification);
            } else {
                toc += sizeof(CADataModification);
            }

            ARCopyShared(header);
        } break;
        case kARSubtypeSystemImage: {
            CAHeaderSystemImage *header = archive->address;
            CADataModification *dataModification = archive->address + header->dataModification;

            toc = archive->address + header->tocOffset;
            ARCopyShared(header);

            if (dataModification->compressionCount || dataModification->encryptionCount)
                fprintf(stderr, "Warning: Archive may contain data modification!\n");
        } break;
        default: success = false; break;
    }

    if (success) success = ARShowContentsInternal(archive->subtype, toc, entryTable, dataOffset, showSize, showLinks);
    return success;
}

// Print SystemImage version string
static OSUTF8Char *ARShowVersionString(CASystemVersionInternal *version)
{
    OSUTF8Char *string;

    OSUTF8Char *systemType;
    OSUTF8Char *buildType;
    OSUTF8Char revision;

    switch (version->type)
    {
        case kCASystemTypeCoronaX: systemType = (OSUTF8Char *)"Corona-X";   break;
        case kCASystemTypeCorOS:   systemType = (OSUTF8Char *)"CorOS";      break;
        default:                   systemType = (OSUTF8Char *)"Unknown OS"; break;
    }

    switch (version->buildType)
    {
        case kCASystemBuildTypeDebug:       buildType = (OSUTF8Char *)"Debug";       break;
        case kCASystemBuildTypeDevelopment: buildType = (OSUTF8Char *)"Development"; break;
        case kCASystemBuildTypeRelease:     buildType = (OSUTF8Char *)"Release";     break;
        case kCASystemBuildTypeStable:      buildType = (OSUTF8Char *)"Stable";      break;
        default:                            buildType = (OSUTF8Char *)"?????";       break;
    }

    revision = 'A' + version->revision;
    asprintf((char **)&string, "%s version %c.%u (%s). Build ID 0x%012lX", systemType, revision, version->majorVersion, buildType, version->buildID);
    return string;
}

// Print archive's header
static void ARShowHeader(ARArchive *archive)
{
    fprintf(stdout, "Archive Signature:     '%.4s'\n", archive->address);
    fprintf(stdout, "CAR Version:           '%.4s'\n", archive->address + 4);

    #define ARSubtype1Shared(h)                                                                     \
        fprintf(stdout, "Entry Table Offset:    %lu\n",     h->entryTableOffset);                   \
        fprintf(stdout, "Data Section Offset:   %lu\n",     h->dataSectionOffset);                  \
        fprintf(stdout, "Data Checksum:         0x%08X\n",  h->dataChecksum);                       \
        fprintf(stdout, "Header Checksum:       0x%08X\n",  h->headerChecksum)

    #define ARSubtype2Shared(h)                                                                     \
        fprintf(stdout, "ToC Offset:            %lu\n",     h->tocOffset);                          \
        ARSubtype1Shared(h);                                                                        \
        fprintf(stdout, "Data Modification:     %lu\n",     h->dataModification);                   \
        fprintf(stdout, "Signature:             %lu\n",     h->archiveSignature)

    #define ARDataModificationShared(h)                                                             \
        fprintf(stdout, "Compression Count:     %hhu\n",     dataModification->compressionCount);   \
        fprintf(stdout, "Encryption Count:      %hhu\n",     dataModification->encryptionCount)

    switch (archive->subtype)
    {
        case kARSubtype1: {
            CAHeaderS1 *header = archive->address;

            fprintf(stdout, "ToC Offset (const):    %lu\n",     sizeof(CAHeaderS1));
            ARSubtype1Shared(header);
        } break;
        case kARSubtype2: {
            CAHeaderS2 *header = archive->address;
            ARSubtype2Shared(header);

            CADataModification *dataModification = archive->address + header->dataModification;
            ARDataModificationShared(dataModification);
        } break;
        case kARSubtypeBootX: {
            CAHeaderBootX *header = archive->address;

            fprintf(stdout, "Boot ID:               0x%08X\n", header->bootID);
            fprintf(stdout, "Processor Type:        0x%04X\n", header->processorType);
            fprintf(stdout, "Lock A:                0x%04X\n", header->lockA);

            ARSubtype1Shared(header);

            fprintf(stdout, "Kernel Loader Entry:   %hu\n", header->kernelLoaderEntry);
            fprintf(stdout, "Kernel Entry:          %hu\n", header->kernelEntry);
            fprintf(stdout, "Boot Config Entry:     %hu\n", header->bootConfigEntry);

            fprintf(stdout, "Lock B:                0x%04X\n", header->lockB);
        } break;
        case kARSubtypeSystemImage: {
            CAHeaderSystemImage *header = archive->address;
            OSUTF8Char *version = ARShowVersionString(&header->systemVersion);

            fprintf(stdout, "System Version:        %s\n", version);
            ARSubtype2Shared(header);
            free(version);

            UInt64 bootEntry = header->bootEntry;

            if (!(~bootEntry)) {
                fprintf(stdout, "Boot Archive Entry:    None\n");
            } else {
                fprintf(stdout, "Boot Archive Entry:    %lu\n", bootEntry);
            }

            CADataModification *dataModification = archive->address + header->dataModification;
            ARDataModificationShared(dataModification);
        } break;
        default: return;
    }
}

bool ARShowInformation(const OSUTF8Char *path, bool showHeader, bool showContents, bool showSize, bool showLinks)
{
    ARArchive *archive = ARArchiveOpen(path);
    if (!archive) return false;

    if (showHeader) ARShowHeader(archive);

    if (showContents)
    {
        fprintf(stdout, "Contents:\n");

        return ARListContentsInternal(archive, showSize, showLinks);
    }

    return ARArchiveClose(archive);
}

bool ARListContents(const OSUTF8Char *path, bool showLinks)
{
    ARArchive *archive = ARArchiveOpen(path);
    if (!archive) return false;

    bool success = ARListContentsInternal(archive, false, showLinks);
    return (ARArchiveClose(archive) && success);
}
