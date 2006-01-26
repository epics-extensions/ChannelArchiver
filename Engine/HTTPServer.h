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

// Base
#include <epicsTime.h>
#include <epicsThread.h>
// Tools
#include <ToolsConfig.h>
#include <NetTools.h>
#include <Guard.h>
#include <AutoPtr.h>
// Engine
#include "HTMLPage.h"

// undef: Nothing
//     1: Start/Stop
//     2: Show Client IPs
//     3: Connect & cleanup
//     4: Requests
//     5: whatever
#define HTTPD_DEBUG 5

// Maximum number of clients that we accept.
// This includes connections that are "done"
// and need to be cleaned up
// (which happens every HTTPD_TIMEOUT seconds)
#define MAX_NUM_CLIENTS 10

// These timeouts influence how quickly the server reacts,
// including how fast the whole engine can be shut down.

// Timeout for server to check for new client
#define HTTPD_TIMEOUT 1

// Client uses timeout for each read
#define HTTPD_READ_TIMEOUT 1

// HTTP clients older than this total timeout are killed
#define HTTPD_CLIENT_TIMEOUT 10

/// \addtogroup Engine
/// @{

/// An in-memory web server.
///
/// Waits for connections on the given TCP port,
/// creates an HTTPClientConnection
/// for each incoming, well, client.
///
/// The HTTPClientConnection then needs to handle
/// the incoming requests and produce appropriate
/// and hopefully strikingly beautiful HTML pages.
class HTTPServer : public epicsThreadRunable
{
public:
    /// Create a HTTPServer.
    /// @parm port The TCP port number wher the server listens.
    /// @return New HTTPServer, needs to be 'start()'ed.
    /// @exception GenericException when port unavailable.
    static HTTPServer *create(short port, void *user_arg);
    
    virtual ~HTTPServer();

    /// Start accepting connections (launch thread).
    void start();

    /// Part of the epicsThreadRunable interface. Do not call directly!
    void run();

    /// Dump HTML page with server info to socket.
    void serverinfo(SOCKET socket);

    void *getUserArg() const
    {   return user_arg; }

    /// @return Returns true if the server wants to shut down.
    bool isShuttingDown() const
    {   return go == false; }    

private:
    HTTPServer(SOCKET socket, void *user_arg);
    epicsThread                           thread;
    bool                                  go;
    SOCKET                                socket;
    void                                  *user_arg;
    size_t                                total_clients;
    double                                client_duration; // seconds; averaged
    epicsMutex                            client_list_mutex;
    AutoArrayPtr< AutoPtr<class HTTPClientConnection> > clients;

    // Create a new HTTPClientConnection, add to 'clients'.
    void start_client(SOCKET peer);

    // returns # of clients that are still active
    size_t client_cleanup();
};


/// Used by HTTPClientConnection to dispatch client requests
///
/// Terminator: entry with path = 0.
typedef struct
{
    typedef void (*PathHandler) (class HTTPClientConnection *connection,
                                 const stdString &full_path,
                                 void *user_arg);
    const char  *path;      // Path for this handler
    size_t      path_len;   // _Relevant_ portion of path to check (if > 0)
    PathHandler handler;    // Handler to call
}   PathHandlerList;

/// Handler for a HTTPServer's client.
///
/// Handles input and dispatches
/// to a PathHandler from PathList.
/// It's deleted when the connection is closed.
class HTTPClientConnection : public epicsThreadRunable
{
public:
    static PathHandlerList  *handlers;

    HTTPClientConnection(HTTPServer *server, SOCKET socket, int num);
    virtual ~HTTPClientConnection();

    HTTPServer *getServer()
    {   return server; }
    
    SOCKET getSocket()
    {   return socket; }
            
    size_t getNum()
    {   return num; }
    
    bool isDone()
    {   return done; }    

    /// Predefined PathHandlers
    void error(const stdString &message);
    void pathError(const stdString &path);
 
    void start()
    {  thread.start(); }

    void run();

    const epicsTime &getBirthTime()
    {   return birthtime; }

    /// @returns The runtime in seconds.
    double getRuntime() const
    {   return runtime; }

private:
    epicsThread              thread; // .. that handles this connection
    HTTPServer              *server;
    epicsTime                birthtime;
    size_t                   num;    // unique sequence number of this conn.
    bool                     done;   // has run() finished running?
    SOCKET                   socket;
    stdVector<stdString>     input_line;
    char                     line[2048];
    unsigned int             dest;  // index of next unused char in _line
    double                   runtime;

    // Result: done, i.e. connection can be closed?
    bool handleInput();

    // return: full request that I can handle?
    bool analyzeInput();

    void dumpInput(HTMLPage &page);
};

/// @}

#endif // !defined(HTTP_SERVER_H_)
