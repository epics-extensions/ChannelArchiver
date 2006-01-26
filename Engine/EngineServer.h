#ifndef __ENGINESERVER_H__
#define __ENGINESERVER_H__

// Engine
#include <HTTPServer.h>

class EngineServer : public HTTPServer
{
public:
        // Constructor creates and starts the HTTPServer
        // for given engine.
	EngineServer(short port, class Engine *engine);

        // Stop and delete the HTTPServer.
	~EngineServer();
};

#endif //__ENGINESERVER_H__
