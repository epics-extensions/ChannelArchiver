// Base
#include <epicsThread.h>

// Tools
#include "OrderedMutex.h"
#include "Guard.h"
#include "MsgLogger.h"

#ifdef DETECT_DEADLOCK

/** Monitor the locks that one thread currently holds. */
class ThreadList
{
public:
    /** Initialize with first thread and lock. */
    ThreadList(epicsThreadId thread, OrderedMutex *lock) : thread(thread)
    {
        add(lock);
    }
    
    /** @return Returns the thread handled by this list. */
    epicsThreadId getThread() const
    {   return thread; }

    /** Add a lock that this thread wants to take. */
    bool add(const OrderedMutex *lock)
    {
        locks.push_back(lock);
        return check();
    }

    /** Remove a lock from list for this thread. */
    void remove(const OrderedMutex *lock)
    {
        locks.remove(lock);
    }

    bool isEmpty() const
    {
        return locks.empty();
    }

    /** Dump info about locks. */
    void dump() const;
    
private:
    epicsThreadId thread;
    stdList<const OrderedMutex *> locks;
    /** Check lock order. */
    bool check() const;
};

void ThreadList::dump() const
{
    char name[100];
    epicsThreadGetName(thread, name, sizeof(name));
    fprintf(stderr, "Thread '%s': ", name);
    bool first = true;
    stdList<const OrderedMutex *>::const_iterator i;
    for (i = locks.begin();  i != locks.end();  ++i)
    {
        const OrderedMutex *l = *i;
        if (first)
            first = false;
        else
            fprintf(stderr, ", ");
        fprintf(stderr, "'%s' (%zu)", l->getName().c_str(), l->getOrder());
    }
    printf("\n");
}

bool ThreadList::check() const
{
    stdList<const OrderedMutex *>::const_iterator i = locks.begin(); 
    if (i == locks.end())
        return true; // empty
    const OrderedMutex *l = *i;
    size_t previous = l->getOrder();
    ++i;
    while (i != locks.end())
    {
        l = *i;
        size_t order = l->getOrder();
        if (order < previous)
            return false;
        previous = order;
        ++i;
    }
    return true;
}

/** Monitor which thread takes which locks. */
class LockMonitor
{
public:
    /** Constructor. There should only be one lock monitor. */
    LockMonitor()
    {
        LOG_ASSERT(! exists);
        exists = true;
    }

    ~LockMonitor()
    {
        LOG_ASSERT(exists);
        exists = false;
    }
    
    /** Record that given thread tries to take some mutex. */
    void add(const char *file, size_t line,
             epicsThreadId thread, OrderedMutex &lock);

    /** Record that given thread release some mutex. */
    void remove(epicsThreadId thread, OrderedMutex &lock);
private:
    static bool exists;
    epicsMutex mutex;
    stdList<ThreadList> threads;
    void dump(Guard &guard);
};

bool LockMonitor::exists = false;


void LockMonitor::add(const char *file, size_t line,
                      epicsThreadId thread, OrderedMutex &lock)
{
    Guard guard(mutex);
    // Add lock to the list of locks for that thread.
    stdList<ThreadList>::iterator i;
    for (i = threads.begin();  i != threads.end();  ++i)
    {
        if (i->getThread() == thread)
        {
            if (!i->add(&lock))
            {
                fprintf(stderr, "=========================================\n");
                fprintf(stderr, "Violation of lock order in\n");
                fprintf(stderr, "file %s, line %zu:\n\n", file, line);
                dump(guard);
                fprintf(stderr, "=========================================\n");
                throw GenericException(file, line, "Violation of lock order");
            }
            return;
        }
    }
    threads.push_back(ThreadList(thread, &lock));
}

void LockMonitor::remove(epicsThreadId thread, OrderedMutex &lock)
{
    Guard guard(mutex);
    // Remove lock from the list of locks for that thread.
    stdList<ThreadList>::iterator i;
    for (i = threads.begin();  i != threads.end();  ++i)
    {
        if (i->getThread() == thread)
        {
            i->remove(&lock);
            if (i->isEmpty())
                threads.erase(i);
            return;
        }
    }
    throw GenericException(__FILE__, __LINE__, "Unknown thread");
}

void LockMonitor::dump(Guard &guard)
{
    // Remove lock from the list of locks for that thread.
    stdList<ThreadList>::iterator i;
    for (i = threads.begin();  i != threads.end();  ++i)
        i->dump();
}

// The one and only lock monitor.
LockMonitor lock_monitor;

void OrderedMutex::lock(const char *file, size_t line)
{
    lock_monitor.add(file, line, epicsThreadGetIdSelf(), *this);
    mutex.lock();
}
    
void OrderedMutex::unlock()
{
    lock_monitor.remove(epicsThreadGetIdSelf(), *this);
    mutex.unlock();
}
#endif

