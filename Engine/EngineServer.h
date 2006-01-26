#ifndef __ENGINESERVER_H__
#define __ENGINESERVER_H__

// Tools
#include <AutoPtr.h>

class EngineServer
{
public:
	EngineServer(short port, class Engine *engine);
	~EngineServer();
private:
        class Engine *engine;
	AutoPtr<class HTTPServer> server;
};

#endif //__ENGINESERVER_H__
