// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "ASCIIParser.h"
#include <ctype.h>

ASCIIParser::ASCIIParser()
{
    _line_no = 0;
    _file = 0;
}

ASCIIParser::~ASCIIParser()
{
    if (_file)
        fclose(_file);
}

bool ASCIIParser::open(const stdString &file_name)
{
    _file = fopen(file_name.c_str(), "rt");
    return _file != 0;
}

bool ASCIIParser::nextLine()
{
#define MAX_LEN	1024
	char line[MAX_LEN];
	char *ch;
	size_t i;

	while (true)
	{
        if (fgets(line, MAX_LEN, _file) == 0)
		{   // got nothing -> quit
            _line.assign((const char *)0, 0);
            return false;
		}
		++_line_no;

		ch = line;
		i = 0;
		// skip leading white space
		while (*ch && isspace(*ch))
		{
			++ch;
			++i;
			if (i >= MAX_LEN)
			{
				_line.assign((const char *)0, 0);
				return false;
			}
		}
		if (! *ch)
			return nextLine(); // empty line

		// skip comment lines
		if (*ch == '#')
			continue; // try next line

        // remove trailing white space
        i = strlen(ch);
        while (i > 0  && isspace(ch[i-1]))
            --i;
        ch[i] = '\0';

		_line = ch;
		return true;
	}
#undef MAX_LEN
}

bool ASCIIParser::getParameter(stdString &parameter, stdString &value)
{
	size_t pos = _line.find('=');
	if (pos == _line.npos)
		return false;

	parameter = _line.substr(0, pos);
	++pos;
	while (_line[pos] && isspace(_line[pos]))
		++pos;
	value = _line.substr (pos);

	return true;
}

// EOF ASCIIParser.cpp
