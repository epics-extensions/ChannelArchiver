// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __CHANNELARCHIVETYPES_H__
#define __CHANNELARCHIVETYPES_H__

#include<MsgLogger.h>
#include<math.h>

// Safely (i.e. w/o overruns and '\0'-limited)
// copy a std-string into a char [].
//
// string::copy isn't available on all platforms, so strncpy is used.
inline void string2cp(char *dest, const stdString &src, size_t maxlen)
{
	if (src.length() >= maxlen)
	{
		LOG_MSG("string2cp: Truncating '%s' to %d chars.\n",
                src.c_str(), maxlen);
		strncpy(dest, src.c_str(), maxlen);
		dest[maxlen-1] = '\0';
	}
	else
		strncpy(dest, src.c_str(), maxlen);
}

#ifndef CPP_EDITION
#error Define CPP_EDITION in ToolsConfig.h
#endif

typedef char Byte;
typedef unsigned long FileOffset;
typedef unsigned short DbrType;
typedef unsigned short DbrCount;

#endif //__CHANNELARCHIVETYPES_H__















