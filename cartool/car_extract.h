#include <System/Archives/OSCAR.h>
#include "car.h"

typedef struct __ARExtractFileInfo {
    const OSUTF8Char *archivePath;
    const OSUTF8Char *resultingPath;
    struct __ARExtractFileInfo *next;
} ARExtractFileInfo;

bool ARExtractFiles(const OSUTF8Char *archive, const OSUTF8Char *rootDirectory, ARExtractFileInfo *files, OSCount fileCount, bool verbose);
bool ARExtractArchive(const OSUTF8Char *archive, const OSUTF8Char *rootDirectory, bool verbose);
