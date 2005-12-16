// -*- c++ -*-
#include "ToolsConfig.h"

#ifdef CONVERSION_REQUIRED
// System
#include<stdint.h>

// EPICS
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
inline void ULONGFromDisk(uint32_t &item)
{	item = ntohl (item);	}

inline void ULONGToDisk(uint32_t &item)
{	item = htonl (item);	}

inline void USHORTFromDisk(uint16_t &item)
{	item = ntohs (item);	}

inline void USHORTToDisk(uint16_t &item)
{ //	item = htons(item);
	uint16_t big_endian;
	uint8_t *p = (uint8_t *)&big_endian;
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

#define SHORTFromDisk(s)	USHORTFromDisk((uint16_t &)s)
#define SHORTToDisk(s)		USHORTToDisk((uint16_t &)s)
#define LONGFromDisk(l)		ULONGFromDisk((uint32_t &)l)
#define LONGToDisk(l)		ULONGToDisk((uint32_t &)l)

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

