// RegularExpression.cpp: implementation of the RegularExpression class.
//////////////////////////////////////////////////////////////////////

#include<ctype.h>
#include"RegularExpression.h"
#include"MsgLogger.h"

//#include<sys/types.h>
extern "C"
{
#ifdef WIN32
#include"gnu_regex.h"
#else
#include<regex.h>
#endif
}

stdString RegularExpression::fromGlobPattern(const stdString &glob)
{
    stdString pattern;
    size_t len = glob.length();

    pattern = "^"; // start anchor
    for (size_t i=0; i<len; ++i)
    {
        switch (glob[i])
        {
        case '*':
            pattern += ".*";
            break;
        case '?':
            pattern += '.';
            break;
        default:
            pattern += '[';   // 'x' -> "[xX]"
            pattern += tolower(glob[i]);
            pattern += toupper(glob[i]);
            pattern += ']';
        }
    }
    pattern += '$'; // end anchor
    
    return pattern;
}

// RegularExpression could have private members
//  regex_t compiled_pattern;
//  bool    is_pattern_valid;
//
// But then RegularExpression.h would have to include <regex.h>
// which is quite big and does not contain extern "C".
// Using a void pointer is also nasty but keeps
// RegularExpression.h minimal.
RegularExpression *RegularExpression::reference(const char *pattern,
                                                bool case_sensitive)
{
    RegularExpression *re = new RegularExpression;
    re->_compiled_pattern = new regex_t;
    int flags = REG_EXTENDED | REG_NOSUB;
    if (! case_sensitive)
        flags |= REG_ICASE;
    
    if (regcomp((regex_t *) re->_compiled_pattern, pattern, flags) != 0)
    {
        delete ((regex_t *)re->_compiled_pattern);
        re->_compiled_pattern = 0;
        delete re;
        return 0;
    }

    return re;
}

RegularExpression::RegularExpression()
 : _compiled_pattern(0), _refs(1)
{ }

RegularExpression::~RegularExpression()
{
    if (_compiled_pattern)
    {
        regfree((regex_t *) _compiled_pattern);
        delete ((regex_t *) _compiled_pattern);
        _compiled_pattern = 0;
    }
}

bool RegularExpression::doesMatch(const char *input)
{
    return regexec((regex_t *) _compiled_pattern, input,
                   /*nmatch*/ 0, /*pmatch[]*/ 0, /*eflags*/ 0) == 0;
}


