// -*- c++ -*-
#include "ArchiverConfig.h"

#ifdef CONVERSION_REQUIRED
#include<osiSock.h>
#include<db_access.h>

// TODO: remove ntohx/htonx calls!
// Profiling Engine/bench.cpp showed that
// the conversion calls consume quite some time.
// Changing USHORTToDisk from htons to a custom
// implementation improved it:
// In a test run writing 1200000 values,
// the htons version took 0.15 seconds,
// the custom code only   0.04 seconds!
inline void ULONGFromDisk(unsigned long &item)
{	item = ntohl (item);	}

inline void ULONGToDisk(unsigned long &item)
{	item = htonl (item);	}

inline void USHORTFromDisk(unsigned short &item)
{	item = ntohs (item);	}

inline void USHORTToDisk(unsigned short &item)
{ //	item = htons(item);
	unsigned short big_endian;
	unsigned char *p = (unsigned char *)&big_endian;
	p[0] = item >> 8;
	p[1] = item & 0xFF;
	item = big_endian;
}

inline void DoubleFromDisk(double &d)
{
    dbr_long_t  cvrt_tmp = ntohl(((dbr_long_t *)&d)[0]);
    ((dbr_long_t *)&d)[0] = ntohl(((dbr_long_t *)&d)[1]);
    ((dbr_long_t *)&d)[1] = cvrt_tmp;
}

inline void DoubleToDisk(double &d)
{
    dbr_long_t  cvrt_tmp = htonl(((dbr_long_t *)&d)[0]);
    ((dbr_long_t *)&d)[0] = htonl(((dbr_long_t *)&d)[1]);
    ((dbr_long_t *)&d)[1] = cvrt_tmp;
}

inline void FloatFromDisk(float &d)
{   *((dbr_long_t *)&d) = ntohl(*((dbr_long_t *)&d)); }

inline void FloatToDisk(float &d)
{   *((dbr_long_t *)&d) = htonl(*((dbr_long_t *)&d)); }

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
#define SHORTToDisk(s)		USHORTToDisk((unsigned short &)s)
#define LONGFromDisk(l)		ULONGFromDisk((unsigned long &)l)
#define LONGToDisk(l)		ULONGToDisk((unsigned long &)l)

#else

#define ULONGFromDisk(s) {}
#define ULONGToDisk(i)   {}
#define USHORTFromDisk(i) {}
#define USHORTToDisk(i)   {}
#define DoubleFromDisk(i) {}
#define DoubleToDisk(i)   {}
#define FloatFromDisk(i)   {}
#define FloatToDisk(i)   {}
#define epicsTimeStampFromDisk(i) {}
#define epicsTimeStampToDisk(i) {}
#define SHORTFromDisk(i) {}
#define SHORTToDisk(i) {}
#define LONGFromDisk(i) {}
#define LONGToDisk(i) {}

#endif // CONVERSION_REQUIRED

#define FileOffsetFromDisk ULONGFromDisk
#define FileOffsetToDisk ULONGToDisk

