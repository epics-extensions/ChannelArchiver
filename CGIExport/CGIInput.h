// CGIInput.cpp

#ifndef CGI_INPUT_H
#define CGI_INPUT_H

#include"CGIDemangler.h"
#include<stdio.h>

class CGIInput : public CGIDemangler
{
public:
    bool parse(FILE *in, FILE *out);
};

#endif
