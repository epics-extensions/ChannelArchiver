// CGIInput.cpp

#ifndef CGI_INPUT_H
#define CGI_INPUT_H

#include "CGIDemangler.h"
#include <iostream>

using std::istream;
using std::ostream;
USING_NAMESPACE_TOOLS

class CGIInput : public CGIDemangler
{
public:
	bool parse (istream &in, ostream &error);
};

#endif
