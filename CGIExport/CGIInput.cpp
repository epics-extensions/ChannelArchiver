// CGIInput.cpp

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "CGIInput.h"

#ifdef WIN32
#define strcasecmp strcmpi
#else
#include <strings.h>
#endif

bool CGIInput::parse(std::istream &in, std::ostream &error)
{
    char *input = 0;

    /** Depending on the request method, read all CGI input into cgiinput **/
    /** (really should produce HTML error messages, instead of exit()ing) **/
    const char *request_method = getenv("REQUEST_METHOD") ;
    if (!request_method)
    {
        error << "REQUEST_METHOD is not provided\n";
        return false;
    }
    
    if (!strcasecmp(request_method, "GET") || !strcasecmp(request_method, "HEAD"))
    {
        const char *query = getenv("QUERY_STRING");
        if (! query)
        {
            error << "GET/HEAD request but QUERY_STRING is not provided\n";
            return false;
        }
        size_t len = strlen(query);
        input = new char[len+1];
        memcpy (input, query, len+1);
    }
    else if (!strcasecmp(request_method, "POST"))
    {
        if (!getenv("CONTENT_TYPE")  || 
            strcasecmp(getenv("CONTENT_TYPE"), "application/x-www-form-urlencoded"))
        {
            error << "CONTENT_TYPE must be application/x-www-form-urlencoded\n" ;
            return false;
        }
        const char *len_str = getenv("CONTENT_LENGTH");
        size_t content_length = len_str ? atoi(len_str) : 0;
        if (! content_length)
        {
            error << "No CONTENT_LENGTH was sent with the POST request.\n";
            return false;
        }
        input = new char [content_length+1];
        in.read (input, content_length);
        input[content_length]='\0' ;
    }
    else
    {
        error << "unsupported REQUEST_METHOD\n";
        return false;
    }

    CGIDemangler::parse(input);

    delete [] input;

    return true;    
}


