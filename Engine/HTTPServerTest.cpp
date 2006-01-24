// Tools
#include <AutoPtr.h>
#include <UnitTest.h>
// Engine
#include "HTTPServer.h"

static void echo(HTTPClientConnection *connection, const stdString &path)
{
    HTMLPage page(connection->getSocket(), "Echo");
    page.line("<H3>Echo</H3>");
    page.line("<PRE>");
    page.line(path);
    page.line("</PRE>");
}

static PathHandlerList  handlers[] =
{
    //  URL, substring length?, handler. The order matters!
    { "/",  0, echo },
    { 0,    0,  },
};

TEST_CASE test_http_server()
{
    HTTPClientConnection::handlers = handlers;
    try
    {
        AutoPtr<HTTPServer> server(HTTPServer::create(4812));
        server->start();
        printf("Server is running, try\n");
        printf("  lynx -dump http://localhost:4812\n");
        epicsThreadSleep(10.0);
    }
    catch (GenericException &e)
    {
        FAIL(e.what());
    }
    TEST_OK;
}

