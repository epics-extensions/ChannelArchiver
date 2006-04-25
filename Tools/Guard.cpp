// System
#include <stdlib.h>
// Base
#include <epicsThread.h>
// Tools
#include "MsgLogger.h"
#include "Guard.h"

void Guard::check(const char *file, size_t line, const epicsMutex &the_one_it_should_be)
{
    if (&mutex == &the_one_it_should_be)
        return;
    if (getenv("ABORT_ON_ERRORS"))
    {
        LOG_MSG("%s (%zu): Found a Guard for the wrong Mutex",
            file, line);
        abort();
    }
    // else
    throw GenericException(file, line, "Found a Guard for the wrong Mutex");
}

void Guard::lock()
{    
#ifdef DEBUG_DEADLOCK
    size_t i = 0;
    while (1)
    {
        if (mutex.tryLock())
            break;
        epicsThreadSleep(0.01);
        if (++i > 1000) // apx. 10 seconds
        {
            LOG_MSG("Assumed deadlock");
            abort();
        }
    }
#else
    mutex.lock();
#endif
    is_locked = true;
}
