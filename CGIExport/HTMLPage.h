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

using std::vector;

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
	stdString _directory;
	stdString _pattern;
	vector<stdString> _names;
	double _round;
	double _interpol;
	bool _fill;
	bool _status;
	osiTime _start;
	osiTime _end;

private:
	bool _started;
};

#endif

