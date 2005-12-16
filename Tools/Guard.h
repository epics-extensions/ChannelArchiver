// -*- c++ -*-

#ifndef _GUARD_H_
#define _GUARD_H_

// Base
#include <epicsMutex.h>
// Tools
#include <GenericException.h>

/// \ingroup Tools Guard automatically takes and releases an epicsMutex.

/// Idea follows Jeff Hill's epicsGuard.
class Guard
{
public:
    /// Constructor attaches to mutex and locks.
    Guard(epicsMutex &mutex) : mutex(mutex)
    {
        mutex.lock();
        is_locked = true;
    }

    /// Destructor unlocks.
    ~Guard()
    {
        if (!is_locked)
            throw GenericException(__FILE__, __LINE__,
                   "Found a released lock in Guard::~Guard()");
        mutex.unlock();
    }

    /// Check if the guard is assigned to the correct mutex.
    void check(const char *file, size_t line, const epicsMutex &the_one_it_should_be)
    {
        if (&mutex != &the_one_it_should_be)
            throw GenericException(file, line, "Found a Guard for the wrong Mutex");
    }

    /// Unlock, meant for temporary, manual unlock().
    void unlock()
    {
        mutex.unlock();
        is_locked = false;
    }

    /// Lock again after a temporary unlock.
    void lock()
    {
        mutex.lock();
        is_locked = true;
    }

private:
    Guard(const Guard &); // not impl.
    Guard &operator = (const Guard &); // not impl.
    epicsMutex &mutex;
    bool is_locked;
};

#endif
