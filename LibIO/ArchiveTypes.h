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

#include <MsgLogger.h>
#include <osiTimeHelper.h>

USING_NAMESPACE_TOOLS

// Safely (i.e. w/o overruns and '\0'-limited)
// copy a std-string into a char [].
//
// string::copy isn't available on all platforms, so strncpy is used.
inline void string2cp (char *dest, const stdString &src, size_t maxlen)
{
	if (src.length() >= maxlen)
	{
		LOG_MSG ("string2cp: Truncating '" << src << "' to " << maxlen << " chars.\n");
		strncpy (dest, src.c_str(), maxlen);
		dest[maxlen-1] = '\0';
	}
	else
		strncpy (dest, src.c_str(), maxlen);
}

#ifndef CPP_EDITION
#error Define CPP_EDITION in ToolsConfig.h
#endif

// Namespace usage is coupled to doing so in Tools:
#ifdef USE_NAMESPACE_TOOLS
#define USE_NAMESPACE_CHANARCH
#endif

#ifdef USE_NAMESPACE_CHANARCH
#	define BEGIN_NAMESPACE_CHANARCH	namespace ChanArch      {
#	define END_NAMESPACE_CHANARCH	}
#	define USING_NAMESPACE_CHANARCH	using namespace ChanArch;
#else
#	define BEGIN_NAMESPACE_CHANARCH
#	define END_NAMESPACE_CHANARCH
#	define USING_NAMESPACE_CHANARCH
#endif

BEGIN_NAMESPACE_CHANARCH

typedef char Byte;

typedef unsigned short DbrType;
typedef unsigned short DbrCount;

END_NAMESPACE_CHANARCH

#endif //__CHANNELARCHIVETYPES_H__
