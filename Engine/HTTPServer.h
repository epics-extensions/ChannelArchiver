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

#include <epicsTime.h>
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
    int getTotalClientCount()
    {   return _total_clients; }

    void serverinfo(SOCKET socket);
    
private:
    HTTPServer(SOCKET socket);
    void cleanup();

    epicsThread                           _thread;
    bool                                  _go;
    SOCKET                                _socket;
    size_t                                _total_clients;
    stdList<class HTTPClientConnection *> _clients;
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
    HTTPClientConnection(HTTPServer *server, SOCKET socket, int num);
    virtual ~HTTPClientConnection();

    HTTPServer *getServer()
    {   return _server; }
    
    SOCKET getSocket()
    {   return _socket; }
            
    int getNum()
    {   return _num; }
    
    bool isDone()
    {   return _done; }
    
    static void setPathHandlers(PathHandlerList *handler)
    {   _handler = handler; }

    // Predefined PathHandlers:
    void error(const stdString &message);
    void pathError(const stdString &path);
 
    void start()
    {  _thread.start(); }
    void run();

    const epicsTime &getBirthTime()
    {   return _birthtime; }

private:
    HTTPServer              *_server;
    epicsTime                _birthtime;
    epicsThread              _thread; // .. that handles this connection
    int                      _num;    // unique sequence number of this conn.
    bool                     _done;   // has run() finished running?
    SOCKET                   _socket;
    stdVector<stdString>     _input_line;
    char                     _line[2048];
    unsigned int             _dest;  // index of next unused char in _line
    static PathHandlerList  *_handler;

    // Result: done, i.e. connection can be closed?
    bool handleInput();

    // return: full request that I can handle?
    bool analyzeInput();

    void dumpInput(HTMLPage &page);
};

#endif // !defined(HTTP_SERVER_H_)
