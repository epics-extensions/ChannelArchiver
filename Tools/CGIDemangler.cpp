// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "CGIDemangler.h"

#ifdef WIN32
#define strcasecmp strcmpi
#endif

// Convert a two-char hex string into the char it represents
static char x2c(const char *what)
{
   char digit;

   digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
   digit *= 16;
   digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
   return digit;
}

// Reduce any %xx escape sequences to the characters they represent
void CGIDemangler::unescape(char *url)
{
    int i,j;

    for(i=0,j=0; url[j]; ++i,++j)
    {
        if((url[i] = url[j]) == '%')
        {
            url[i] = x2c(&url[j+1]) ;
            j+= 2 ;
        }
    }
    url[i] = '\0' ;
}

void CGIDemangler::unescape(stdString &text)
{
    size_t len = text.length() + 1;
    char *buf = (char *)malloc(len);
    if (!buf)
        return;
    memcpy(buf, text.c_str(), len);
    unescape(buf);
    text = buf;
    free(buf);
}

void CGIDemangler::analyseVar(char *var)
{
    char *eq;

    // name / value
    if ((eq=strchr(var, '=')))
    {
        *eq = '\0';
        unescape (var);
        unescape (eq+1);
        stdString name, value;
        name = var;
        value = eq+1;
        add (name, value);
    }
}

void CGIDemangler::parse (char *input)
{
    size_t i;

    // Change all plusses back to spaces
    for(i=0; input[i]; i++)
        if (input[i] == '+') input[i] = ' ' ;

    // Split on "&" to extract the name-value pairs
    char *var = strtok (input, "&");
    while (var)
    {
        analyseVar (var);
        var = strtok (NULL, "&");
    }
}

void CGIDemangler::add (const stdString &name, const stdString &value)
{
    _vars.insert (stdMap<stdString, stdString>::value_type(name, value));
}

stdString CGIDemangler::find (const stdString &name) const
{
    stdMap<stdString, stdString>::const_iterator pos = _vars.find (name);
    if (pos != _vars.end())
        return pos->second;
    else
        return "";
}

