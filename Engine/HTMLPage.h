// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __HTMLPAGE_H__
#define __HTMLPAGE_H__

#ifdef solaris
// Hack around clash of struct map in inet headers with std::map
#define map xxxMapxxx
#endif
#include <osiSock.h>
#ifdef solaris
#undef map
#endif

/**\ingroup Engine
 * Helper for printing a web page to a socket.
 */
class HTMLPage
{
public:
    /** Create web page for socket with given title.
     *  <p>
     *  Maybe also with a reload-refresh every given seconds.
     */
    HTMLPage (SOCKET socket, const char *title, int refresh=0);

    virtual ~HTMLPage ();

    /** Add a line. */
    void line (const char *line);

    /** Add a line. */
    void line (const stdString &line);

    /** Add verbatim output. */
    void out (const char *line);

    /** Add verbatim output. */
    void out (const char *line, size_t length);

    /** Add verbatim output. */
    void out (const stdString &line);

    /** Start a table with column headers.
     *  <p>
     *  Arguments: size, name, size, name, ...., 0.
     *  <p>
     *  Last column name must be 0.
     */
    void openTable (int colspan, const char *column, ...);
    
    /** Add line to table.
     *  <p>
     *  Number of items, ending in 0, must match call to openTable().
     */
    void tableLine (const char *item, ...);
    
    /** End a table. */
    void closeTable ();

    /** Should bottom menu include a 'config'  link? */
    static bool with_config;
protected:
    SOCKET socket;
    int    refresh;
};

#endif //__HTMLPAGE_H__
