#ifndef __ENGINESERVER_H__
#define __ENGINESERVER_H__

class EngineServer
{
public:
	EngineServer ();
	~EngineServer ();

	static short _port;
private:
	class HTTPServer *_server;
};

#endif //__ENGINESERVER_H__
