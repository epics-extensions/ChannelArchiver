#ifndef __BINTYPES_H__
#define __BINTYPES_H__

#include "ArchiveTypes.h"
#include <LowLevelIO.h>

// Types used by the Channel Archiver
// and conversion routines to transform
// from and into disk format (=Motorola network format)
//
// CONVERSION_REQUIRED is automatically set in net_convert.h:
#include <net_convert.h>
#ifdef CONVERSION_REQUIRED
#include <osiSock.h>
#endif

#ifdef CONVERSION_REQUIRED

inline void ULONGFromDisk(unsigned long &item)
{	item = ntohl (item);	}

inline void ULONGToDisk(unsigned long &item)
{	item = htonl (item);	}

inline void USHORTFromDisk(unsigned short &item)
{	item = ntohs (item);	}

inline void USHORTToDisk(unsigned short &item)
{	item = htons (item);	}

inline void TS_STAMPFromDisk (TS_STAMP &ts)
{
	ts.secPastEpoch = ntohl (ts.secPastEpoch);
	ts.nsec = ntohl (ts.nsec);
}

inline void TS_STAMPToDisk (TS_STAMP &ts)
{
	ts.secPastEpoch = ntohl (ts.secPastEpoch);
	ts.nsec = ntohl (ts.nsec);
}

inline void DoubleToDisk (double &d)
{	dbr_htond (&d, &d);	}

inline void DoubleFromDisk (double &d)
{	dbr_ntohd (&d, &d);	}

inline void FloatToDisk (float &d)
{	dbr_htonf (&d, &d);	}

inline void FloatFromDisk (float &d)
{	dbr_ntohf (&d, &d);	}

#else // ! CONVERSION_REQUIRED

#define ULONGFromDisk(item)	/**/
#define ULONGToDisk(item)	/**/
#define USHORTFromDisk(item)	/**/
#define USHORTToDisk(item)	/**/
#define TS_STAMPFromDisk(item)	/**/
#define TS_STAMPToDisk(item)	/**/
#define DoubleToDisk(item)	/**/
#define DoubleFromDisk(item)	/**/
#define FloatToDisk(item)	/**/
#define FloatFromDisk(item)	/**/

#endif // CONVERSION_REQUIRED

#define SHORTFromDisk(s)	USHORTFromDisk((unsigned short &)s)
#define LONGFromDisk(l)		ULONGFromDisk((unsigned long &)l)
#define SHORTToDisk(s)		USHORTToDisk((unsigned short &)s)
#define LONGToDisk(l)		ULONGToDisk((unsigned long &)l)

// used internally for offsets inside files:
const FileOffset INVALID_OFFSET = 0xffffffff;
#define FileOffsetFromDisk ULONGFromDisk
#define FileOffsetToDisk ULONGToDisk

inline bool isValidFilename (const stdString &name)
{	return ! name.empty();	}

inline bool isValidFilename (const char *name)
{	return name[0] != '\0';	}

#endif // __BINTYPES_H__
