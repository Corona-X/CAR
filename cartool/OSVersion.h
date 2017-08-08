/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* OSVersion.h - Data Structures for reading System Version        */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/
/* beeselmane - 27.11.2015 - 8:00 PM EST                           */
/**=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=**/

#ifndef __SYSTEM_OSVERSION__
#define __SYSTEM_OSVERSION__

#include "Corona-X.h"
#include "OSCAR.h"

#if !kCXAssemblyCode

typedef CXEnum(UInt16, OSBuildType) {
    kOSBuildTypeDebug       = kCARSystemBuildTypeDebug,
    kOSBuildTypeDevelopment = kCARSystemBuildTypeDevelopment,
    kOSBuildTypeRelease     = kCARSystemBuildTypeRelease,
    kOSBuildTypeStable      = kCARSystemBuildTypeStable
};

typedef CXEnum(UInt16, OSSystemType) {
    kOSTypeCoronaX          = kCARSystemTypeCoronaX,
    kOSTypeCorOS            = kCARSystemTypeCorOS,
    kOSTypeXMobile          = kCARSystemTypeXMobile,
    kOSTypeCMobile          = kCARSystemTypeCMobile
};

#endif /* !kCXAssemblyCode */

#endif /* !defined(__SYSTEM_OSVERSION__) */
