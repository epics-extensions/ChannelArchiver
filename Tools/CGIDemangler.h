// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef CGI_DEMANGLER_H
#define CGI_DEMANGLER_H

#include <ToolsConfig.h>

//CLASS CGIDemangler
// De-mangle CGI-type input,
// e.g. the QUERY string for CGI scripts
// or the GET/POST text that a web server
// receives.
//
// All variables (name/value pairs) are placed in a std::map.
//
// For a class that actually reads cin or QUERY_STRING,
// see CLASS CGIInput.
//
class CGIDemangler
{
public:
    // Input will be altered (but not extended) !
    void parse (char *input);

    //* Demangle string input
    void parse (const char *input)
    {
        size_t len = strlen(input)+1;
        char *safe_copy = new char[len];
        memcpy (safe_copy, input, len);
        parse (safe_copy);
        delete [] safe_copy;
    }

    //* Manually add another name/value to map
    void add (const stdString &name, const stdString &value);
    
    //* Query map
    stdString find (const stdString &name) const;

    //* Retrieve full map
    const stdMap<stdString, stdString> &getVars () const
    {   return _vars;   }

private:
    stdMap<stdString, stdString>    _vars;

    void analyseVar (char *var);
};

#endif


