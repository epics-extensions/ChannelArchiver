// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "HTTPServer.h"
#include "MsgLogger.h"

// HTTPServer ----------------------------------------------


HTTPServer::HTTPServer(SOCKET socket)
        : _thread(*this, "HTTPD",
                  epicsThreadGetStackSize(epicsThreadStackBig),
                  epicsThreadPriorityMedium)
{
#ifdef ENGINE_DEBUG
    LOG_MSG("new HTTPServer\n");
#endif
    _socket = socket;
    _go = true;
}

HTTPServer::~HTTPServer()
{
    _go = false;
    if (! _thread.exitWait(5.0))
        LOG_MSG("HTTPServer: server thread does not exit\n");
    if (_socket)
        socket_close(_socket);
}

HTTPServer *HTTPServer::create(short port)
{
#ifdef ENGINE_DEBUG
    LOG_MSG("new HTTPServer\n");
#endif
    
    SOCKET s = socket (AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in  local;

    local.sin_family = AF_INET;
    local.sin_port = htons (port);
#   ifdef WIN32
    local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#   else
    local.sin_addr.s_addr = htonl(INADDR_ANY);
#   endif
    int val = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&val, sizeof(val)))
    {
        LOG_MSG("setsockopt(SO_REUSEADDR) failed for HTTPServer::create\n");
    }

    if (bind(s, (const struct sockaddr *)&local, sizeof local) != 0)
    {
        LOG_MSG("bind failed for HTTPServer::create\n");
        return 0;
    }
    listen(s, 3);

#ifdef ENGINE_DEBUG
    LOG_MSG("HTTPServer socket ready, creating HTTPServer\n");
#endif
    
    return new HTTPServer(s);
}

void HTTPServer::start()
{
    _thread.start();
}

void HTTPServer::run()
{
    fd_set fds;
    struct timeval timeout;
    struct sockaddr_in  peername;
    socklen_t  len;

#ifdef ENGINE_DEBUG
    LOG_MSG("HTTPServer thread 0x%08X running\n", epicsThreadGetIdSelf());
#endif
    while (_go)
    {
        // somebody there?
        FD_ZERO(&fds);
        FD_SET(_socket, &fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if (select(_socket+1, &fds, 0, 0, &timeout) == 1  &&
            FD_ISSET(_socket, &fds))
        {
            len = sizeof peername;
            SOCKET peer = accept(_socket, (struct sockaddr *)&peername, &len);
            if (peer != INVALID_SOCKET)
            {
#ifdef ENGINE_DEBUG
                stdString local_info, peer_info;
                GetSocketInfo(peer, local_info, &peer_info);
                LOG_MSG("HTTPServer thread 0x%08X accepted %s/%s\n",
                        epicsThreadGetIdSelf(),
                        local_info, peer_info);
#endif
                HTTPClientConnection *client = new HTTPClientConnection(peer);
                client->start();
            }
        }
    }
#ifdef ENGINE_DEBUG
    LOG_MSG("HTTPServer thread 0x%08X exiting\n", epicsThreadGetIdSelf());
#endif
}

// HTTPClientConnection -----------------------------------------

// statics:
PathHandlerList *HTTPClientConnection::_handler;
size_t HTTPClientConnection::_total = 0;
size_t HTTPClientConnection::_clients = 0;

HTTPClientConnection::HTTPClientConnection(SOCKET socket)
        : _thread(*this, "HTTPClientConnection",
                  epicsThreadGetStackSize(epicsThreadStackBig),
                  epicsThreadPriorityLow)
{
    _socket = socket;
    _dest = 0;
    ++_total;
    ++_clients;
}

HTTPClientConnection::~HTTPClientConnection()
{
    if (_clients > 0)
        --_clients;
    else
    {
        LOG_MSG("~HTTPClientConnection: odd client count\n");
    }
    socket_close(_socket);
}

void HTTPClientConnection::start()
{
    _thread.start();
}


void HTTPClientConnection::run()
{
    while (handleInput())
    {
    }
    delete this;
}

// Result: done, i.e. connection can be closed?
bool HTTPClientConnection::handleInput()
{
    // gather input into lines:
    char stuff[200];
    int end = recv(_socket, stuff, sizeof stuff-1, 0);
    char *src;
    
    if (end < 0) // client changed his mind?
        return true;
    
    if (end > 0)
    {
        stuff[end] = 0;
        for (src = stuff; *src; ++src)
        {
            switch (*src)
            {
                case 13:
                    if (*(src+1) == 10)
                        continue; // handle within next loop
                    // else: handle EOL now
                case 10:
                    // complete current line
                    _line[_dest] = 0;
                    _input_line.push_back(_line);
                    _dest = 0;
                    break;
                default:
                    // avoid writing past end of _line!!!
                    if (_dest < (sizeof(_line)-1))
                        _line[_dest++] = *src;
            }
        }
    }
    return analyzeInput();
}

void HTTPClientConnection::dumpInput(HTMLPage &page)
{
    page.line("<OL>");
    for (size_t i=0; i<_input_line.size(); ++i)
    {
        page.out ("<LI>");
        page.line (_input_line[i]);
    }
    page.line ("</OL>");
}

// return: full request that I can handle?
bool HTTPClientConnection::analyzeInput()
{
    // format: GET <path> HTTP/1.1
    // need first line to determine this:
    if (_input_line.size() <= 0)
        return false;

    if (_input_line[0].substr(0,3) != "GET")
    {
        HTMLPage page (_socket, "Error");
        page.line("<H1>Error</H1>");
        page.line("Cannot handle this input:");
        dumpInput(page);
        return true;
    }

    size_t pos = _input_line[0].find("HTTP/");
    if (pos == _input_line[0].npos)
    {
        LOG_MSG("pre-HTTP 1.0 GET line: '%s'\n", _input_line[0].c_str());
        HTMLPage page(_socket, "Error");
        page.line("<H1>Error</H1>");
        page.line("Pre-HTTP 1.0 GET is not supported:");
        dumpInput(page);
        return true;
    }

    // full requests ends with empty line:
    if (_input_line.back().length() > 0)
        return false;

    stdString path = _input_line[0].substr(4, pos-5);
    stdString protocol = _input_line[0].substr(pos+5);
    try
    {   // Linear search ?!
        // Hash or map lookup isn't straightforward
        // because path has to be cut for parameters first...
        PathHandlerList *h;
        
        for (h = _handler; h && h->path; ++h)
        {
            if ((h->path_len > 0  &&
                 strncmp(path.c_str(), h->path, h->path_len) == 0)
                ||
                path == h->path)
            {
#               ifdef LOG_HTTP
                stdString peer;
                GetSocketPeer (_socket, peer);
                LOG_MSG("HTTP get '" << path
                        << "' from " << peer << "\n");
#               endif
                h->handler(this, path);
                return true;
            }
        }
        pathError(path);
    }
    catch (GenericException &e)
    {
        LOG_MSG("HTTPClientConnection: Error in PathHandler for '%s':\n%s\n",
                path.c_str(), e.what());
    }
    return true;
}

void HTTPClientConnection::error(const stdString &message)
{
    HTMLPage page(_socket, "Error");
    page.line(message);
}

void HTTPClientConnection::pathError (const stdString &path)
{
    error("The path <I>'" + path + "'</I> you asked for does not exist.");
}
