/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* OSCX.h - Standard Structures for CX Executable Format           */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* beeselmane - 27.11.2015 - 10:00 AM EST                          */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/

#ifndef __SYSTEM_EXECUTABLES_OSCX__
#define __SYSTEM_EXECUTABLES_OSCX__ 1

#include "Corona-X.h"
#include "OSTypes.h"

#if !kCXAssemblyCode

#define kCXProcessorTypeMask64Bit   (1 << 15)

#define kCXProcessorTypeX86         0
#define kCXProcessorTypeX86_64      (kCXProcessorTypeMask64Bit | kCXProcessorTypeX86)
#define kCXProcessorTypeARMv7       1
#define kCXProcessortypeARMv8       (kCXProcessorTypeMask64Bit  | kCXProcessorTypeARMv7)

#endif /* !kCXAssemblyCode */

#endif /* !defined(__SYSTEM_EXECUTABLES_OSCX__) */
