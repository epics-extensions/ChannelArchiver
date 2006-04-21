// -*- c++ -*-

#ifndef _GUARD_H_
#define _GUARD_H_

// Base
#include <epicsMutex.h>
// Tools
#include <GenericException.h>

#define DEBUG_DEADLOCK

/** \ingroup Tools
 *  Interface for something that can be protected by a Guard.
 *
 *  @see Guard
 */
class Guardable
{
public:
    /** @return Returns the mutex for this object. */
    virtual epicsMutex &getMutex() = 0;
};

/** \ingroup Tools
 *  Guard automatically takes and releases an epicsMutex.
 *
 *  Idea follows Jeff Hill's epicsGuard.
 */
class Guard
{
public:
    /** Constructor attaches to mutex and locks. */
    Guard(Guardable &guardable) : mutex(guardable.getMutex()), is_locked(false)
    {
        lock();
        is_locked = true;
    }

    /** Constructor attaches to mutex and locks. */
    Guard(epicsMutex &mutex) : mutex(mutex), is_locked(false)
    {
        lock();
        is_locked = true;
    }

    /** Destructor unlocks. */
    ~Guard()
    {
        if (!is_locked)
            throw GenericException(__FILE__, __LINE__,
                   "Found a released lock in Guard::~Guard()");
        mutex.unlock();
    }

    /** Check if the guard is assigned to the correct mutex.
     *  <p>
     *  Uses ABORT_ON_ERRORS environment variable.
     */
    void check(const char *file, size_t line,
               const epicsMutex &the_one_it_should_be);

    /** Unlock, meant for temporary, manual unlock(). */
    void unlock()
    {
        mutex.unlock();
        is_locked = false;
    }

    /** Lock again after a temporary unlock. */
    void lock();

    /** @return Returns the current lock state. */
    bool isLocked()
    {
        return is_locked;
    }

private:
    Guard(const Guard &); // not impl.
    Guard &operator = (const Guard &); // not impl.
    epicsMutex &mutex;
    bool is_locked;
};


/** \ingroup Tools
 *  Temporarily releases and then re-takes a Guard.
 *
 *  Idea follows Jeff Hill's epicsGuard.
 */
class GuardRelease
{
public:
    /** Constructor releases the guard. */
    GuardRelease(Guard &guard) : guard(guard)
    {
        guard.unlock();
    }

    /** Destructor re-locks the guard. */
    ~GuardRelease()
    {
        guard.lock();
    }

private:
    Guard &guard;
};

#endif
