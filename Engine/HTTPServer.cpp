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

#include "ArchiverConfig.h"
#include "HTTPServer.h"
#include "MsgLogger.h"

// HTTPServer ----------------------------------------------

HTTPServer::HTTPServer(SOCKET socket)
        : thread(*this, "HTTPD",
                 epicsThreadGetStackSize(epicsThreadStackBig),
                 epicsThreadPriorityMedium),
          go(true), socket(socket)
{}

HTTPServer::~HTTPServer()
{
    go = false;
    if (! thread.exitWait(5.0))
        LOG_MSG("HTTPServer: server thread does not exit\n");
    epicsSocketDestroy(socket);
#ifdef HTTPD_DEBUG
    LOG_MSG("HTTPServer::~HTTPServer\n");
#endif
}

HTTPServer *HTTPServer::create(short port)
{
    SOCKET s = epicsSocketCreate(AF_INET, SOCK_STREAM, 0);
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
    thread.start();
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
    while (go)
    {
        // Don't hang in accept() but use select() so that
        // we have a chance to react to the main program
        // setting _go == false
        FD_ZERO(&fds);
        FD_SET(socket, &fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if (select(socket+1, &fds, 0, 0, &timeout) == 1  &&
            FD_ISSET(socket, &fds))
        {
            len = sizeof peername;
            SOCKET peer = accept(socket, (struct sockaddr *)&peername, &len);
            if (peer != INVALID_SOCKET)
            {
#if             defined(HTTPD_DEBUG)  && HTTPD_DEBUG > 1
                stdString local_info, peer_info;
                GetSocketInfo(peer, local_info, peer_info);
                LOG_MSG("HTTPServer thread 0x%08X accepted %s/%s\n",
                        epicsThreadGetIdSelf(),
                        local_info.c_str(), peer_info.c_str());
#endif
                ++total_clients;
                HTTPClientConnection *client =
                    new HTTPClientConnection(this, peer, total_clients);
                clients.push_back(client);
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
    stdList<HTTPClientConnection *>::iterator ci;

#   if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 2
    LOG_MSG("-----------------------------------------------------\n");
    LOG_MSG("HTTPServer thread 0x%08X clients: %d total\n",
            epicsThreadGetIdSelf(), total_clients);
    for (ci = clients.begin(); ci != clients.end(); ++ci)
    {
        client = *ci;
        LOG_MSG("HTTPClientConnection #%d: %s\n",
                client->getNum(),
                (client->isDone() ? "done" : "running"));
    }
    LOG_MSG("-----------------------------------------------------\n");
#   endif

    ci = clients.begin();
    epicsTime now = epicsTime::getCurrent();
    while (ci != clients.end())
    {
        client = *ci;
        if (client->isDone())
        {
            ci = clients.erase(ci);
#           if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 1
            LOG_MSG("HTTPClientConnection cleanup of #%d (done)\n",
                    client->getNum());
#           endif
            delete client;
        }
        else if ((now - client->getBirthTime()) > HTTPD_CLIENT_TIMEOUT)
        {
            ci = clients.erase(ci);
#           if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 1
            LOG_MSG("HTTPClientConnection cleanup of #%d (timeout)\n",
                    client->getNum());
#           endif
            delete client;
        }
        else
            ++ci;
    }
}

void HTTPServer::serverinfo(SOCKET socket)
{
    HTMLPage page(socket, "Server Info");
    
    HTTPClientConnection *  client;
    stdList<HTTPClientConnection *>::iterator ci;

    page.openTable(3, "Clients", 0);
    page.tableLine("Client Number", "Status", "Age [secs]", 0);
    char num[20], age[50];
    const char *status;
    epicsTime now = epicsTime::getCurrent();
    for (ci = clients.begin(); ci != clients.end(); ++ci)
    {
        client = *ci;
        sprintf(num, "%d", client->getNum());
        status = (client->isDone() ? "done" : "running");
        sprintf(age, "%f", now - client->getBirthTime());
        page.tableLine(num, status, age, 0);
    }
    page.closeTable();
}


// HTTPClientConnection -----------------------------------------

// statics:
PathHandlerList *HTTPClientConnection::handlers;

HTTPClientConnection::HTTPClientConnection(HTTPServer *server,
                                           SOCKET socket, int num)
        : thread(*this, "HTTPClientConnection",
                 epicsThreadGetStackSize(epicsThreadStackBig),
                 epicsThreadPriorityLow),
          server(server), num(num), done(false), socket(socket)
{
    dest = 0;
    birthtime = epicsTime::getCurrent();
}

HTTPClientConnection::~HTTPClientConnection()
{
    if (! done)
    {
        LOG_MSG("HTTPClientConnection: Shutdown of #%d", num);
        shutdown(socket, SHUT_RDWR);
        epicsSocketDestroy(socket);
    }
}

void HTTPClientConnection::run()
{
#if 0
    stdString local_info, peer_info;
    GetSocketInfo(socket, local_info, peer_info);
    LOG_MSG("HTTPClientConnection #%d thread 0x%08X, handles %s/%s\n",
            num, epicsThreadGetIdSelf(),
            local_info.c_str(), peer_info.c_str());
#endif

    // Unclear:
    // The final 'socket_close' call should flush
    // queued write requests.
    // But especially with Mozilla, some pages show up
    // incomplete even though this thread has printed
    // all the data.
    // SO_LINGER makes us hang in the close call for 30 seconds.
    // Seems to help some but doesn't solve the problem 100% of the time.
    struct linger linger;
    linger.l_onoff = 1;
    linger.l_linger = 30;
    setsockopt(socket, SOL_SOCKET, SO_LINGER,
               (const char *)&linger, sizeof(linger));
    while (!handleInput())
    {
    }
    // The delay and shutdown seem to help Mozilla
    // to read the web page before it stops because
    // the connection quits.
    epicsThreadSleep(2.0);
    shutdown(socket, 2);
    epicsSocketDestroy(socket);
    done = true;
}

// Result: done, i.e. connection can be closed?
bool HTTPClientConnection::handleInput()
{
    // gather input into lines:
    char stuff[100];
    int end = recv(socket, stuff, (sizeof stuff)-1, 0);
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
                    line[dest] = 0;
                    input_line.push_back(line);
                    dest = 0;
                    break;
                default:
                    // avoid writing past end of _line!!!
                    if (dest < (sizeof(line)-1))
                        line[dest++] = *src;
            }
        }
    }
    return analyzeInput();
}

void HTTPClientConnection::dumpInput(HTMLPage &page)
{
    page.line("<OL>");
    for (size_t i=0; i<input_line.size(); ++i)
    {
        page.out ("<LI>");
        page.line (input_line[i]);
    }
    page.line ("</OL>");
}

// return: full request that I can handle?
bool HTTPClientConnection::analyzeInput()
{
    // format: GET <path> HTTP/1.1
    // need first line to determine this:
    if (input_line.size() <= 0)
        return false;

    if (input_line[0].substr(0,3) != "GET")
    {
        LOG_MSG("HTTP w/o GET line: '%s'\n", input_line[0].c_str());
        HTMLPage page (socket, "Error");
        page.line("<H1>Error</H1>");
        page.line("Cannot handle this input:");
        dumpInput(page);
        return true;
    }

    size_t pos = input_line[0].find("HTTP/");
    if (pos == input_line[0].npos)
    {
        LOG_MSG("pre-HTTP 1.0 GET line: '%s'\n", input_line[0].c_str());
        HTMLPage page(socket, "Error");
        page.line("<H1>Error</H1>");
        page.line("Pre-HTTP 1.0 GET is not supported:");
        dumpInput(page);
        return true;
    }

    // full requests ends with empty line:
    if (input_line.back().length() > 0)
        return false;

#   if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 4
    LOG_MSG("HTTPClientConnection::analyzeInput\n");
    for (size_t i=0; i<input_line.size(); ++i)
    {
        LOG_MSG("line %d: '%s'\n",
                i, input_line[i].c_str());
    }
#   endif
    
    stdString path = input_line[0].substr(4, pos-5);
    stdString protocol = input_line[0].substr(pos+5);
        // Linear search ?!
        // Hash or map lookup isn't straightforward
        // because path has to be cut for parameters first...
        if (strcmp(path.c_str(), "/server") == 0)
        {
            server->serverinfo(socket);
            return true;
        }
        PathHandlerList *h;
        for (h = handlers; h && h->path; ++h)
        {
            if ((h->path_len > 0  &&
                 strncmp(path.c_str(), h->path, h->path_len) == 0)
                ||
                path == h->path)
            {
#               if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 4
                stdString peer;
                GetSocketPeer(socket, peer);
                LOG_MSG("HTTP get '%s' from %s\n", path.c_str(), peer.c_str());
#               endif
                h->handler(this, path);
#               if defined(HTTPD_DEBUG) && HTTPD_DEBUG > 4
                LOG_MSG("HTTP invoked handler for '%s'\n", path.c_str());
#               endif
                return true;
            }
        }
        pathError(path);
    return true;
}

void HTTPClientConnection::error(const stdString &message)
{
    HTMLPage page(socket, "Error");
    page.line(message);
}

void HTTPClientConnection::pathError (const stdString &path)
{
    error("The path <I>'" + path + "'</I> does not exist.");
}
