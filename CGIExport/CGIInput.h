// CGIInput.cpp

#ifndef CGI_INPUT_H
#define CGI_INPUT_H

#include "CGIDemangler.h"
#include <iostream>

#ifdef USE_NAMESPACE_STD
using std::istream;
using std::ostream;
#endif
USING_NAMESPACE_TOOLS

class CGIInput : public CGIDemangler
{
public:
	bool parse (istream &in, ostream &error);
};

#endif
