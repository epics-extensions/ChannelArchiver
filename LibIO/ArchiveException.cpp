// ChannelArchiveException.cpp
//////////////////////////////////////////////////////////////////////

#include "ArchiveException.h"
#include <strstream>

static const char *error_text[] =
{
/* NoError */		"NoError: This should never occur...",
/* Fail  */			"Failure",
/* Invalid */		"Invalid",
/* OpenError */		"Cannot open file",
/* CreateError*/	"Cannot create new file",
/* ReadError */		"Read Error",
/* WriteError */	"Write Error",
/* Unsupported */	"Not Supported",
};

const char *ArchiveException::what() const
{
	if (_error_info.empty ())
	{
        std::strstream buf;

		if (_detail.empty())
			buf << getSourceFile() << " (" << getSourceLine() <<"): "
					<< error_text[_code] << '\0';
		else
			buf << getSourceFile() << " (" << getSourceLine() <<"): "
					<< error_text[_code]
					<< "\n(" << _detail << ')' << '\0';
		_error_info = buf.str ();
		buf.rdbuf()->freeze (false);
	}

	return _error_info.c_str ();
}


