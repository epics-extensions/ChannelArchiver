// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef HTML_PAGE_H
#define HTML_PAGE_H

#include <vector>
#include "osiTime.h"
#include "ToolsConfig.h"

using stdVector;

class HTMLPage
{
public:
    HTMLPage ();
    virtual ~HTMLPage ();

    void start ();
    void header (const stdString &text, int level) const;

    // Print the interface stuff
    void interFace () const;

    // Page context
    stdString _title;
    stdString _cgi_path;
    stdString _command;
    stdString _format;
    stdString _directory;
    stdString _pattern;
    stdVector<stdString> _names;
    double _interpol;
    bool _glob;
    bool _fill;
    bool _status;
    bool _reduce;
    bool _use_logscale;
    osiTime _start;
    osiTime _end;
    double _y0, _y1;

private:
    bool _started;
};

#endif

