// -*- c++ -*-
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// kasemir@lanl.gov

#if !defined(HTTP_SERVER_H_)
#define HTTP_SERVER_H_

#include <epicsThread.h>
#include <ToolsConfig.h>
#include <NetTools.h>
#include "HTMLPage.h"

//CLASS
// A HTTPServer creates a CLASS HTTPClientConnection
// for each incoming, well, client.
//
// The HTTPClientConnection then needs to handle
// the incoming requests and produce appropriate
// and hopefully strikingly beatiful HTML pages.
class HTTPServer : public epicsThreadRunable
{
public:
    //* Create a HTTPServer.
    static HTTPServer *create(short port);
    virtual ~HTTPServer();

    //* Start accepting connections
    // (launch thread)
    void start();

    void run();

private:
    HTTPServer(SOCKET socket);

    epicsThread _thread;
    bool        _go;
    SOCKET      _socket;
};

typedef void (*PathHandler) (class HTTPClientConnection *connection,
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
class HTTPClientConnection : public epicsThreadRunable
{
public:
    HTTPClientConnection(SOCKET socket);
    virtual ~HTTPClientConnection();

    void start();
    
    SOCKET getSocket()
    {   return _socket; }

    static void setPathHandlers(PathHandlerList *handler)
    {   _handler = handler; }

    // List of all open client connections
    static size_t getClientCount()
    {   return _clients;    }

    // Total number of clients since started (for debugging)
    static size_t getTotalClientCount()
    {   return _total;  }

    // Predefined PathHandlers:
    void error(const stdString &message);
    void pathError(const stdString &path);

    void run();

private:
    epicsThread              _thread;
    SOCKET                   _socket;
    stdVector<stdString>     _input_line;
    char                     _line[2048];
    unsigned int             _dest; // index in line
    static PathHandlerList  *_handler;
    static size_t _total;
    static size_t _clients;

    // Result: done, i.e. connection can be closed?
    bool handleInput();

    // return: full request that I can handle?
    bool analyzeInput();

    void dumpInput(HTMLPage &page);
};

#endif // !defined(HTTP_SERVER_H_)
