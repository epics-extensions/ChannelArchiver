// HTMLPage

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

	void start (const stdString &title);
	void header (const stdString &text, int level);

	// Print the interface stuff
	void interFace (const stdString &cgi, const stdString &directory, const stdString &pattern,
		const vector<stdString> &names, double round, bool fill, bool status,
		osiTime &start, osiTime &end, bool today = false);

private:
	bool _started;
};

#endif

