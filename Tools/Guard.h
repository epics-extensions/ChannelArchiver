// -*- c++ -*-

#ifndef _GUARD_H_
#define _GUARD_H_

// Base
#include <epicsMutex.h>
// Tools
#include <MsgLogger.h>

/// \ingroup Tools Guard automatically takes and releases an epicsMutex.

/// Meant to be used like Jeff Hill's epicsGuard,
/// but uses LOG_ASSERT instead of throwing exceptions.
/// Also no template but fixed for epicsMutex.
class Guard
{
public:
    /// Constructor attaches to mutex and locks.
    Guard(epicsMutex &mutex)
            : mutex(mutex)
    {
        mutex.lock();
    }

    /// Destructor unlocks.
    ~Guard()
    {
        mutex.unlock();
    }

    //void releaseBriefly()
    //{
    //    mutex.unlock();
    //    mutex.lock();
    //}

    /// Check if the guard is assigned to the correct mutex.
    void check(const epicsMutex &the_one_it_should_be)
    {
        LOG_ASSERT(&mutex == &the_one_it_should_be);
    }
private:
    Guard(const Guard &); // not impl.
    Guard &operator = (const Guard &); // not impl.
    epicsMutex &mutex;
};

#endif
