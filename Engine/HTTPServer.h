// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#if !defined(HTTP_SERVER_H_)
#define HTTP_SERVER_H_

#include <list>
#include <vector>
#include <osiSock.h>
#include <fdManager.h>
#include "HTMLPage.h"

#ifdef USE_NAMESPACE_STD
using std::list;
using std::vector;
#endif

//CLASS
// A HTTPServer creates a CLASS HTTPClientConnection
// for each incoming, well, client.
//
// All these classes are essentially run
// by the EPICS fdManager, so it's process() method
// has to be called periodically.
class HTTPServer : public fdReg
{
public:
    //* Only user-callable method in HTTPServer:<BR>
    // Create a HTTPServer.
    static HTTPServer *create (short port=4812);
    virtual ~HTTPServer();

private:
    HTTPServer (SOCKET socket);
    void callBack ();

    SOCKET  _socket;
};

class HTTPClientConnection;
typedef void (*PathHandler) (HTTPClientConnection *connection,
                             const stdString &full_path);

// Used by HTTPClientConnection
// to dispatch client requests
//
// Terminator: entry with path = 0
typedef struct
{
    const char  *path;      // Path for this handler
    size_t      path_len;   // _Relevant_ portion of path to check (if > 0)
    PathHandler handler;    // Handler to call
}   PathHandlerList;

//CLASS HTTPClientConnection
// HTTPClientConnection handles input and dispatches
// to a PathHandler from PathList.
// It's deleted when the connection is closed.
class HTTPClientConnection : public fdReg
{
public:
    HTTPClientConnection (SOCKET socket);
    virtual ~HTTPClientConnection ();

    SOCKET getSocket ()
    {   return _socket; }

    static void setPathHandlers (PathHandlerList *handler)
    {   _handler = handler; }

    // List of all open client connections
    static size_t getClientCount ()
    {   return _clients;    }

    // Total number of clients since started (for debugging)
    static size_t getTotalClientCount ()
    {   return _total;  }

    // Predefined PathHandlers:
    void error (const stdString &message);
    void pathError (const stdString &path);

private:
    SOCKET              _socket;
    vector<stdString>   _input_line;
    char                _line[200];
    char                *_dest;

    static PathHandlerList  *_handler;
    static size_t _total;
    static size_t _clients;

    // Called by fdManager on input:
    void callBack ();

    // Result: done, i.e. connection can be closed?
    bool handleInput ();

    // return: full request that I can handle?
    bool analyzeInput ();

    void dumpInput (HTMLPage &page);
};

#endif // !defined(HTTP_SERVER_H_)
