
#include <signal.h>
#include <MsgLogger.h>
#include <osiTimeHelper.h>
#include "ArchiveException.h"
#include "WriteThread.h"
#include "Engine.h"

USING_NAMESPACE_CHANARCH

int WriteThread::run ()
{
	LOG_MSG ("WriteThread started\n");
	while (_go)
	{
		_wait.take ();
		if (_go)
		{
			_writing = true;
			try
			{
				theEngine->writeArchive (_now);
			}
			catch (ArchiveException &e)
			{
				LOG_MSG (osiTime::getCurrent() << ": WriteThread Cannot write, got\n\t" << e.what() << "\n");
				_go = false;
			}
			_writing = false;
		}
	}
	return 0;
}
