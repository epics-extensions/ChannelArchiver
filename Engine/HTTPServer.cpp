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

#include "../ArchiverConfig.h"
#include "HTTPServer.h"
#include "MsgLogger.h"

// HTTPServer ----------------------------------------------


HTTPServer::HTTPServer(SOCKET socket)
        : _thread(*this, "HTTPD",
                  epicsThreadGetStackSize(epicsThreadStackBig),
                  epicsThreadPriorityMedium)
{
    _socket = socket;
    _go = true;
}

HTTPServer::~HTTPServer()
{
    _go = false;
    if (! _thread.exitWait(5.0))
        LOG_MSG("HTTPServer: server thread does not exit\n");
    socket_close(_socket);
#ifdef HTTPD_DEBUG
    LOG_MSG("HTTPServer::~HTTPServer\n");
#endif
}

HTTPServer *HTTPServer::create(short port)
{
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

#ifdef HTTPD_DEBUG
    LOG_MSG("HTTPServer thread 0x%08X running\n", epicsThreadGetIdSelf());
#endif
    while (_go)
    {
        // Don't hang in accept() but use select() so that
        // we have a chance to react to the main program
        // setting _go == false
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
#if 0
                stdString local_info, peer_info;
                GetSocketInfo(peer, local_info, peer_info);
                LOG_MSG("HTTPServer thread 0x%08X accepted %s/%s\n",
                        epicsThreadGetIdSelf(),
                        local_info.c_str(), peer_info.c_str());
#endif
                ++_total_clients;
                HTTPClientConnection *client =
                    new HTTPClientConnection(this, peer, _total_clients);
                _clients.push_back(client);
                cleanup();
                client->start();
            }
        }
    }
    cleanup();
#ifdef HTTPD_DEBUG
    LOG_MSG("HTTPServer thread 0x%08X exiting\n", epicsThreadGetIdSelf());
#endif
}

void HTTPServer::cleanup()
{
    HTTPClientConnection *  client;
    stdList<HTTPClientConnection *>::iterator clients;

#   ifdef HTTPD_DEBUG
    LOG_MSG("-----------------------------------------------------\n");
    LOG_MSG("HTTPServer thread 0x%08X clients: %d total\n",
            epicsThreadGetIdSelf(), _total_clients);
    for (clients = _clients.begin(); clients != _clients.end(); ++clients)
    {
        client = *clients;
        LOG_MSG("HTTPClientConnection #%d: %s\n",
                client->getNum(),
                (client->isDone() ? "done" : "running"));
    }
    LOG_MSG("-----------------------------------------------------\n");
#   endif

    for (clients = _clients.begin(); clients != _clients.end(); ++clients)
    {
        client = *clients;
        if (client->isDone())
        {
            clients = _clients.erase(clients);
            LOG_MSG("HTTPClientConnection cleanup of #%d\n", client->getNum());
            delete client;
        }
    }
}

// HTTPClientConnection -----------------------------------------

// statics:
PathHandlerList *HTTPClientConnection::_handler;

HTTPClientConnection::HTTPClientConnection(HTTPServer *server,
                                           SOCKET socket, int num)
        : _thread(*this, "HTTPClientConnection",
                  epicsThreadGetStackSize(epicsThreadStackBig),
                  epicsThreadPriorityLow)
{
    _server = server;
    _num = num;
    _done = false;
    _socket = socket;
    _dest = 0;
}

HTTPClientConnection::~HTTPClientConnection()
{
#ifdef HTTPD_DEBUG
    LOG_MSG("HTTPClientConnection::~HTTPClientConnection #%d\n", _num);
#endif
}

void HTTPClientConnection::run()
{
#if 0
    stdString local_info, peer_info;
    GetSocketInfo(_socket, local_info, peer_info);
    LOG_MSG("HTTPClientConnection #%d thread 0x%08X, handles %s/%s\n",
            _num, epicsThreadGetIdSelf(),
            local_info.c_str(), peer_info.c_str());
#endif
    while (!handleInput())
    {
    }
        
    socket_close(_socket);
    _done = true;
}

// Result: done, i.e. connection can be closed?
bool HTTPClientConnection::handleInput()
{
    // gather input into lines:
    char stuff[100];
    int end = recv(_socket, stuff, (sizeof stuff)-1, 0);
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
        LOG_MSG("HTTP w/o GET line: '%s'\n", _input_line[0].c_str());
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

#   if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 4
    LOG_MSG("HTTPClientConnection::analyzeInput\n");
    for (size_t i=0; i<_input_line.size(); ++i)
    {
        LOG_MSG("line %d: '%s'\n",
                i, _input_line[i].c_str());
    }
#   endif
    
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
#               ifdef HTTPD_DEBUG
                stdString peer;
                GetSocketPeer(_socket, peer);
                LOG_MSG("HTTP get '%s' from %s\n",
                        path.c_str(), peer.c_str());
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
    error("The path <I>'" + path + "'</I> does not exist.");
}
