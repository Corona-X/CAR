#include <getopt.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "car_create.h"
#include "car_extract.h"
#include "car_show.h"
#include "car.h"

#include <System/Archives/OSCAR.h>

// cartool:
//   -c: create archive [root directory, archive name]
//         -v: verbose
//         --subtype <1, 2, BootX, SystemImage>: select archive subtype
//         --apply-compression <LZMA, LZO>: equivament to --compress-section ToC <type> --compress-section Entries <type> --compress-section Data <type>
//         --compress-section {ToC|EntryTable|DataSection, LZMA|LZO}: compress a given section with the given compression type
//         --apply-encryption <AES, Serpent>: encrypt the archive. Encrypts all data except the header.
//         --sign <certificate>
//
//         --arch <x86_64|ARMv8>: architecture for Boot-X file
//         --bootID <id>: boot ID hex value
//         --kernel-loader <path>: specify kernel loader path
//         --kernel <path>: specify kernel file
//         --boot-config <path>: specify boot config file
//
//         --os-type <Corona-X|CorOS>: specify system type
//         --os-major-version <0-128>: specify major system version
//         --os-revision <character>: specify system revision
//         --build-type <debug|development|release|stable>: specify system build type
//         --build-id <id>: specify system build id
//         --partition-info <path>: read partition flag information from the given file
//         --boot-archive <path>: specify boot archive path
//   -x: extract archive [archive path]
//         -v: verbose
//         -d: output directory
//         -o: output path(s)
//         -f: file(s)
//   -s: show archive contents [archive path(s)]
//         --show-header: show information about the archive header
//         --show-entries: show in-depth information about archive entries
//         --show-size: show the size of each entry
//         --show-links: show link location
//   -l: list paths in archive [archive path(s)]
//         --show-links: show link location
//   -u: show this menu

const char *program_name;

static __attribute__((always_inline)) char *string_without_extension(const char *input)
{
    if (!input) return NULL;

    const char *slash = strrchr(input, '/');
    const char *dot = strrchr(input, '.');
    size_t length = strlen(input);
    char *copy = malloc(length);
    if (!copy) return NULL;
    strcpy(copy, input);

    if (!dot)
        return copy;

    if (!slash)
        slash = input;

    off_t slash_offset = slash - input;
    off_t dot_offset = dot - input;

    if (slash_offset > dot_offset)
        return copy;

    copy[dot_offset] = 0;
    return copy;
}

__attribute__((noreturn)) static void do_usage(bool print_usage, const char *error_msg, ...)
{
    va_list args;
    va_start(args, error_msg);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, error_msg, args);
    va_end(args);

    if (print_usage)
    {
        fprintf(stderr, "Usage: %s <action> <arguments>         \n\n", program_name);
        fprintf(stderr, "Where action is one of the following:  \n");
        fprintf(stderr, "  -c: Create archive                   \n");
        fprintf(stderr, "  -x: Extract archive                  \n");
        fprintf(stderr, "  -s: Show archive contents            \n");
        fprintf(stderr, "  -l: List entries in archive          \n");
        fprintf(stderr, "  -u: Show extended usage menu         \n");
    }

    exit(EXIT_FAILURE);
}

__attribute__((noreturn)) static void do_create(int argc, const char *const *argv)
{
    if (argc < 2)
        do_usage(true, "Not enough arguments!\n");

    // This is my first time using getopt_long...
    // can you tell? ;)
    const struct option options[] = {
        {
            .name = "verbose",
            .has_arg = no_argument,
            .flag = NULL,
            .val = 'v'
        }, {
            .name = "subtype",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 's'
        }, {
            .name = "apply-compression",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'a'
        }, {
            .name = "compress-section",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'c'
        }, {
            .name = "apply-encryption",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'e'
        }, {
            .name = "sign",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'q'
        }, {
            .name = "arch",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'h'
        }, {
            .name = "bootID",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'b'
        }, {
            .name = "kernel-loader",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'l'
        }, {
            .name = "kernel",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'k'
        }, {
            .name = "boot-config",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'f'
        }, {
            .name = "os-type",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'y'
        }, {
            .name = "os-major-version",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'm'
        }, {
            .name = "os-revision",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'r'
        }, {
            .name = "build-type",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 't'
        }, {
            .name = "build-id",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'i'
        }, {
            .name = "partition-info",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'p'
        }, {
            .name = "boot-archive",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'B'
        },{NULL, 0, NULL, 0}
    };

    CASystemVersionInternal system_version;
    ARSubtype subtype = kARSubtypeInvalid;
    ARCreateDataModifiers data_modifiers;
    UInt16 architecture = -1;
    UInt32 boot_id = -1;

    const OSUTF8Char *partition_info = NULL;
    const OSUTF8Char *kernel_loader = NULL;
    const OSUTF8Char *boot_archive = NULL;
    const OSUTF8Char *boot_config = NULL;
    const OSUTF8Char *kernel = NULL;
    bool has_error = false;
    bool verbose = false;
    char c;

    while ((c = getopt_long(argc, (char *const *)argv, "vs:a:c:e:q:h:b:l:k:f:y:m:r:t:i:p:", options, NULL)) != -1)
    {
        switch (c)
        {
            case 'v': verbose = true; break;
            case 's': {
                if (!strcmp("1", optarg)) {
                    subtype = kARSubtype1;
                } else if (!strcmp("2", optarg)) {
                    subtype = kARSubtype2;
                } else if (!strcmp("BootX", optarg)) {
                    subtype = kARSubtypeBootX;
                } else if (!strcmp("SystemImage", optarg)) {
                    subtype = kARSubtypeSystemImage;
                } else {
                    do_usage(true, "Invalid subtype '%s'!\n", optarg);
                }
            } break;
            case 'a': {
                if (subtype == kARSubtype1)
                    do_usage(true, "Subtype 1 archives cannot be compressed!\n");

                data_modifiers.compressEntries = true;
                data_modifiers.compressData = true;
                data_modifiers.compressToC = true;

                if (!strcmp("LZMA", optarg)) {
                    data_modifiers.compressionType = kCACompressionTypeLZMA;
                } else if (!strcmp("LZO", optarg)) {
                    data_modifiers.compressionType = kCACompressionTypeLZO;
                } else {
                    do_usage(true, "Invalid compression type '%s'!\n", optarg);
                }
            } break;
            case 'c': {
                if (subtype == kARSubtype1)
                    do_usage(true, "Subtype 1 archives cannot be compressed!\n");

                if (optind + 1 >= argc)
                    do_usage(true, "Not enough arguments!\n");

                const char *compressionType = argv[optind];
                optind++;

                if (!strcmp("ToC", optarg)) {
                    data_modifiers.compressToC = true;
                } else if (!strcmp("EntryTable", optarg)) {
                    data_modifiers.compressEntries = true;
                } else if (!strcmp("DataSection", optarg)) {
                    data_modifiers.compressData = true;
                } else {
                    do_usage(true, "Invalid section name '%s'!\n", optarg);
                }

                if (!strcmp("LZMA", compressionType)) {
                    data_modifiers.compressionType = kCACompressionTypeLZMA;
                } else if (!strcmp("lZO", compressionType)) {
                    data_modifiers.compressionType = kCACompressionTypeLZO;
                } else {
                    do_usage(true, "Invalid compression type '%s'!\n", compressionType);
                }
            } break;
            case 'e': {
                if (subtype == kARSubtype1)
                    do_usage(true, "Subtype 1 archives cannot be encrypted!\n");

                data_modifiers.encryptArchive = true;

                if (!strcmp("AES", optarg)) {
                    data_modifiers.encryptionType = kCAEncryptionTypeAES;
                } else if (!strcmp("Serpent", optarg)) {
                    data_modifiers.encryptionType = kCAEncryptionTypeSerpent;
                } else {
                    do_usage(true, "Invalid encryption type '%s'!\n", optarg);
                }
            } break;
            case 'q': {
                if (subtype == kARSubtype1)
                    do_usage(true, "Subtype 1 archives cannot be signed!\n");

                data_modifiers.signingCertificate = (const OSUTF8Char *)optarg;
            } break;
            case 'h': {
                if (subtype != kARSubtypeBootX)
                    do_usage(true, "Non-BootX archives cannot have an architecture!\n");

                if (!strcmp("x86_64", optarg)) {
                    architecture = kCAProcessorTypeX86_64;
                } else if (!strcmp("ARMv8", optarg)) {
                    architecture = kCAProcessorTypeARMv8;
                } else {
                    do_usage(true, "Invalid architecture '%s'!\n", optarg);
                }
            } break;
            case 'b': {
                if (subtype != kARSubtypeBootX)
                    do_usage(true, "Non-BootX archives cannot have a BootID!\n");

                char *endptr = NULL;
                boot_id = (UInt32)strtol(optarg, &endptr, 0);

                if (!boot_id || *endptr)
                    do_usage(true, "Invalid BootID '%s'!\n", optarg);
            } break;
            case 'l': {
                if (subtype != kARSubtypeBootX)
                    do_usage(true, "Non-BootX archives don't need a kernel loader!\n");

                kernel_loader = (const OSUTF8Char *)optarg;
            } break;
            case 'k': {
                if (subtype != kARSubtypeBootX)
                    do_usage(true, "Non-BootX archives don't need a kernel!\n");

                kernel = (const OSUTF8Char *)optarg;
            } break;
            case 'f': {
                if (subtype != kARSubtypeBootX)
                    do_usage(true, "Non-BootX archives don't need a boot config!\n");

                boot_config = (const OSUTF8Char *)optarg;
            } break;
            case 'y': {
                if (subtype != kARSubtypeSystemImage)
                    do_usage(true, "Only System Images can hold an OS type!\n");

                if (!strcmp("Corona-X", optarg)) {
                    system_version.type = kCASystemTypeCoronaX;
                } else if (!strcmp("CorOS", optarg)) {
                    system_version.type = kCASystemTypeCorOS;
                } else {
                    do_usage(true, "Invalid OS type '%s'!\n", optarg);
                }
            } break;
            case 'm': {
                if (subtype != kARSubtypeSystemImage)
                    do_usage(true, "Only System Images can hold an OS major version!\n");

                if (strlen(optarg) == 1 && !optarg)
                {
                    system_version.majorVersion = 0;
                    break;
                }

                char *endptr = NULL;
                system_version.majorVersion = strtol(optarg, &endptr, 0);

                if (!system_version.majorVersion || endptr[0])
                    do_usage(true, "Invalid OS major version '%s'\n", optarg);
            } break;
            case 'r': {
                if (subtype != kARSubtypeSystemImage)
                    do_usage(true, "Only System Images can hold an OS revision!\n");

                if (strlen(optarg) > 1 || (optarg[0] < 'A' || optarg[0] > 'Z'))
                    do_usage(true, "Invalid OS major version '%s'\n", optarg);

                system_version.revision = optarg[0] - 'A';
            } break;
            case 't': {
                if (subtype != kARSubtypeSystemImage)
                    do_usage(true, "Only System Images can hold a build type!\n");

                if (!strcmp("debug", optarg)) {
                    system_version.buildType = kCASystemBuildTypeDebug;
                } else if (!strcmp("development", optarg)) {
                    system_version.buildType = kCASystemBuildTypeDevelopment;
                } else if (!strcmp("release", optarg)) {
                    system_version.buildType = kCASystemBuildTypeRelease;
                } else if (!strcmp("stable", optarg)) {
                    system_version.buildType = kCASystemBuildTypeStable;
                } else {
                    do_usage(true, "Invalid build type '%s'!\n", optarg);
                }
            } break;
            case 'i': {
                if (subtype != kARSubtypeSystemImage)
                    do_usage(true, "Only System Images can hold a build ID!\n");

                char *endptr = NULL;
                system_version.buildID = strtol(optarg, &endptr, 0);

                if (!system_version.buildID && optarg[0])
                    do_usage(true, "Invalid Build ID '%s'!\n", optarg);
            } break;
            case 'p': {
                if (subtype != kARSubtypeSystemImage)
                    do_usage(true, "Only System Images need partition info!\n");

                partition_info = (const OSUTF8Char *)optarg;
            } break;
            case 'B': {
                if (subtype != kARSubtypeSystemImage)
                    do_usage(true, "Only System Images can have a Boot Archive!\n");

                boot_archive = (const OSUTF8Char *)optarg;
            } break;
            case '?': {
                fprintf(stderr, "Warning: Encountered unknown option '%c'\n", optopt);
                fprintf(stderr, "Will ignore.\n");
            } break;
        }
    }

    argv += optind;
    argc -= optind;

    if (argc < 2)
        do_usage(true, "Not enough arguments!\n");

    const OSUTF8Char *root_directory = (const OSUTF8Char *)argv[0];
    const OSUTF8Char *archive = (const OSUTF8Char *)argv[1];

    switch (subtype)
    {
        case kARSubtype1: {
            has_error = ARCreateSubtype1(root_directory, archive, verbose);
        } break;
        case kARSubtype2: {
            has_error = ARCreateSubtype2(root_directory, archive, verbose, &data_modifiers);
        } break;
        case kARSubtypeBootX: {
            has_error = ARCreateBootX(root_directory, archive, verbose, &data_modifiers, architecture, boot_id, kernel_loader, kernel, boot_config);
        } break;
        case kARSubtypeSystemImage: {
            has_error = ARCreateSystemImage(root_directory, archive, verbose, &data_modifiers, &system_version, partition_info, boot_archive);
        } break;
        default:
            do_usage(true, "Cannot create an archive with no subtype!\n");
    }

    exit(has_error);
}

__attribute__((noreturn)) static void do_extract(int argc, const char *const *argv)
{
    if (argc < 1)
        do_usage(true, "Not enough arguments!\n");

    OSUTF8Char *output_directory = (OSUTF8Char *)string_without_extension(argv[1]);
    const OSUTF8Char *archive = (const OSUTF8Char *)argv[1];
    ARExtractFileInfo *filelist = NULL;
    bool custom_output = false;
    OSCount file_count = 0;
    bool has_error = false;
    bool verbose = false;
    argv++, argc--;
    char c;

    if (!output_directory)
        do_usage(false, "Out of memory!\n");

    while ((c = getopt(argc, (char *const *)argv, "vd:of")) != -1)
    {
        switch (c)
        {
            case 'v': verbose = true; break;
            case 'd': {
                free(output_directory);

                output_directory = (OSUTF8Char *)optarg;
                custom_output = true;
            } break;
            case 'f': {
                const char *const *argument = argv + optind;

                while ((*argument)[0] != '-' && optind < argc)
                {
                    file_count++;
                    argument++;
                    optind++;
                }

                filelist = malloc(file_count * sizeof(ARExtractFileInfo));
                argument -= file_count;

                if (!filelist)
                    do_usage(false, "Out of memory!\n");

                for (OSIndex i = 0; i < file_count; i++)
                {
                    filelist[i].archivePath = (const OSUTF8Char *)argument;
                    filelist[i].next = filelist + i + 1;
                    filelist[i].resultingPath = NULL;

                    argument++;
                }

                filelist[file_count - 1].next = NULL;
            } break;
            case 'o': {
                if (!filelist) do_usage(true, "A file list must come before an output list!");

                const char *const *argument = argv + optind;
                ARExtractFileInfo *file = filelist;

                while (file->next)
                {
                    file->resultingPath = (const OSUTF8Char *)argument;

                    if ((++optind) >= argc)
                        do_usage(true, "Have files without output files!\n");

                    if ((*(++argument))[0] == '-')
                        do_usage(true, "Have files without output files!\n");

                    file = file->next;
                }

                if (!(optind >= argc) && (*argument)[0] != '-')
                    do_usage(true, "Have excess output files!\n");
            } break;
            case '?': {
                fprintf(stderr, "Warning: Encountered unknown option '%c'\n", optopt);
                fprintf(stderr, "Will ignore.\n");
            } break;
        }
    }

    if (filelist) {
        has_error = ARExtractFiles(archive, output_directory, filelist, file_count, verbose);

        free(filelist);
    } else {
        has_error = ARExtractArchive(archive, output_directory, verbose);
    }

    if (!custom_output)
        free(output_directory);

    exit(has_error);
}

__attribute__((noreturn)) static void do_show(int argc, const char *const *argv)
{
    int show_header = true, show_entries = false, show_size = false, show_links = false;
    bool has_error = false;

    const struct option options[5] = {
        {
            .name = "show-header",
            .has_arg = no_argument,
            .flag = &show_header,
            .val = true
        }, {
            .name = "show-entries",
            .has_arg = no_argument,
            .flag = &show_entries,
            .val = true
        }, {
            .name = "show-size",
            .has_arg = no_argument,
            .flag = &show_size,
            .val = true
        }, {
            .name = "show-links",
            .has_arg = no_argument,
            .flag = &show_links,
            .val = true
        }, {NULL, 0, NULL, 0}
    };

    char c;

    while ((c = getopt_long_only(argc, (char *const *)argv, "", options, NULL)) != -1)
    {
        // Everything should be handled for us automatically!
        // Unless it's not...

        if (c == '?')
        {
            fprintf(stderr, "Warning: Encountered unknown option '%c'\n", optopt);
            fprintf(stderr, "Will ignore.\n");
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1)
        do_usage(true, "Not enough arguments!\n");

    for (uint32_t i = 0; i < argc; i++)
    {
        fprintf(stderr, "Showing archive %s:\n", argv[i]);

        if (!ARShowInformation((const OSUTF8Char *)argv[i], show_header, show_entries, show_size, show_links))
        {
            fprintf(stderr, "Encountered an error!\n");
            has_error = true;
        }
    }

    exit(has_error);
}

__attribute__((noreturn)) static void do_list(int argc, const char *const *argv)
{
    bool show_links = !strcmp(argv[0], "--show-links");
    bool has_error = false;

    if (show_links)
    {
        if (argc < 2)
            do_usage(true, "Not enough arguments!\n");

        argv++;
        argc--;
    }

    for (uint32_t i = 0; i < argc; i++)
    {
        fprintf(stderr, "Entries in archive %s:\n", argv[i]);

        if (!ARListContents((const OSUTF8Char *)argv[i], show_links))
        {
            fprintf(stderr, "Encountered an error!\n");
            has_error = true;
        }
    }

    exit(has_error);
}

__attribute__((noreturn)) static void do_extended_usage(void)
{
    fprintf(stderr, "Usage: %s <action> <arguments>         \n\n", program_name);
    fprintf(stderr, "Where action is one of the following:  \n");

    fprintf(stderr, "-c: create archive [root directory, archive name]\n");
    fprintf(stderr, "      -v: verbose\n");
    fprintf(stderr, "      --subtype <1, 2, BootX, SystemImage>: select archive subtype\n");
    fprintf(stderr, "      --apply-compression <LZMA, LZO>: equivament to --compress-section ToC <type> --compress-section Entries <type> --compress-section Data <type>\n");
    fprintf(stderr, "      --compress-section {ToC|EntryTable|DataSection, LZMA|LZO}: compress a given section with the given compression type\n");
    fprintf(stderr, "      --apply-encryption <AES, Serpent>: encrypt the archive. Encrypts all data except the header.\n");
    fprintf(stderr, "      --sign <certificate>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "      --arch <x86_64|ARMv8>: architecture for Boot-X file\n");
    fprintf(stderr, "      --bootID <id>: boot ID hex value\n");
    fprintf(stderr, "      --kernel-loader <path>: specify kernel loader path\n");
    fprintf(stderr, "      --kernel <path>: specify kernel file\n");
    fprintf(stderr, "      --boot-config <path>: specify boot config file\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "      --os-type <Corona-X|CorOS>: specify system type\n");
    fprintf(stderr, "      --os-major-version <0-128>: specify major system version\n");
    fprintf(stderr, "      --os-revision <character>: specify system revision\n");
    fprintf(stderr, "      --build-type <debug|development|release|stable>: specify system build type\n");
    fprintf(stderr, "      --build-id <id>: specify system build id\n");
    fprintf(stderr, "      --partition-info <path>: read partition flag information from the given file\n");
    fprintf(stderr, "      --boot-archive <path>: specify boot archive path\n");
    fprintf(stderr, "-x: extract archive [archive path]\n");
    fprintf(stderr, "      -v: verbose\n");
    fprintf(stderr, "      -d: output directory\n");
    fprintf(stderr, "      -o: output path(s)\n");
    fprintf(stderr, "      -f: file(s)\n");
    fprintf(stderr, "-s: show archive contents [archive path(s)]\n");
    fprintf(stderr, "      --show-header: show information about the archive header\n");
    fprintf(stderr, "      --show-entries: show in-depth information about archive entries\n");
    fprintf(stderr, "      --show-size: show the size of each entry\n");
    fprintf(stderr, "      --show-links: show link location\n");
    fprintf(stderr, "-l: list paths in archive [archive path(s)]\n");
    fprintf(stderr, "      --show-links: show link location\n");
    fprintf(stderr, "-u: show this menu\n");

    exit(EXIT_SUCCESS);
}

int main(int argc, const char *const *argv)
{
    program_name = basename((char *)argv[0]);

    if (!program_name)
        program_name = argv[0];

    if (argc < 2)
        do_usage(true, "Not enough arguments!\n");

    size_t first_arg_length = strlen(argv[1]);

    if (first_arg_length != 2)
        do_usage(true, "Invalid first argument!\n");

    if (argv[1][0] != '-')
        do_usage(true, "Invalid first argument!\n");

    switch (argv[1][1])
    {
        case 'c':  do_create(argc - 1, argv + 1);
        case 'x': do_extract(argc - 1, argv + 1);
        case 's':    do_show(argc - 1, argv + 1);
        case 'l':    do_list(argc - 2, argv + 2);
        case 'u': do_extended_usage();
        default: do_usage(true, "Invalid first argument!\n");
    }
}
