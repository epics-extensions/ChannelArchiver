#ifndef __BINTYPES_H__
#define __BINTYPES_H__

#include "ArchiveTypes.h"
#include <epicsTime.h>
#include <LowLevelIO.h>

#ifdef CONVERSION_REQUIRED
#include<osiSock.h>

inline void ULONGFromDisk(unsigned long &item)
{	item = ntohl (item);	}

inline void ULONGToDisk(unsigned long &item)
{	item = htonl (item);	}

inline void USHORTFromDisk(unsigned short &item)
{	item = ntohs (item);	}

inline void USHORTToDisk(unsigned short &item)
{	item = htons (item);	}

inline void epicsTimeStampFromDisk(epicsTimeStamp &ts)
{
	ts.secPastEpoch = ntohl(ts.secPastEpoch);
	ts.nsec = ntohl(ts.nsec);
}

inline void epicsTimeStampToDisk(epicsTimeStamp &ts)
{
	ts.secPastEpoch = ntohl(ts.secPastEpoch);
	ts.nsec = ntohl(ts.nsec);
}

#define SHORTFromDisk(s)	USHORTFromDisk((unsigned short &)s)
#define LONGFromDisk(l)		ULONGFromDisk((unsigned long &)l)
#define SHORTToDisk(s)		USHORTToDisk((unsigned short &)s)
#define LONGToDisk(l)		ULONGToDisk((unsigned long &)l)

#else

#define ULONGFromDisk(s) {}
#define ULONGToDisk(i)   {}
#define USHORTFromDisk(i) {}
#define USHORTToDisk(i)   {}
#define epicsTimeStampFromDisk(i) {}
#define epicsTimeStampToDisk(i) {}
#define SHORTFromDisk(i) {}
#define LONGFromDisk(i) {}
#define SHORTToDisk(i) {}
#define LONGToDisk(i) {}

#endif // CONVERSION_REQUIRED

// used internally for offsets inside files:
const FileOffset INVALID_OFFSET = 0xffffffff;
#define FileOffsetFromDisk ULONGFromDisk
#define FileOffsetToDisk ULONGToDisk

inline bool isValidFilename(const stdString &name)
{	return ! name.empty();	}

inline bool isValidFilename(const char *name)
{	return name[0] != '\0';	}

#endif // __BINTYPES_H__
