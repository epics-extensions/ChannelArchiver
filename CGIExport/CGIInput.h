// CGIInput.cpp

#ifndef CGI_INPUT_H
#define CGI_INPUT_H

#include "CGIDemangler.h"
#include <iostream>

class CGIInput : public CGIDemangler
{
public:
    bool parse (std::istream &in, std::ostream &error);
};

#endif
