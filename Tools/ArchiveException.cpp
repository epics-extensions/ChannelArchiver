// ChannelArchiveException.cc
//////////////////////////////////////////////////////////////////////

#include"ArchiveException.h"

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
        char buffer[2048];
		if (_detail.empty())
            sprintf(buffer, "%s (%d): %s\n",
                    getSourceFile(), getSourceLine(), error_text[_code]);
        else
            sprintf(buffer, "%s (%d): %s\n(%s)",
                    getSourceFile(), getSourceLine(), error_text[_code],
                    _detail.c_str());
        _error_info = buffer;
	}

	return _error_info.c_str ();
}


