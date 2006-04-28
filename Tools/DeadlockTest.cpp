// Base
#include <epicsMutex.h>
#include <epicsThread.h>

// Tools
#include "ToolsConfig.h"
#include "MsgLogger.h"
#include "Guard.h"
#include "UnitTest.h"

/** A mutex with informational name and lock order.
 *  <p>
 *  Meant to help with deadlock-detection.
 *  Attempt to take mutexes out of order,
 *  which could result in deadlocks,
 *  are detected.
 */
class OrderedMutex
{
public:
    /** Create mutex with name and lock order. */
    OrderedMutex(const char *name, size_t order);

    /** @return Returns the mutex name. */
    const stdString &getName() const
    {   return name; }

    /** @return Returns the mutex order. */
    size_t getOrder() const
    {   return order; }

    /** Lock the mutex. */
    void lock(const char *file, size_t line);
    
    /** Unlock the mutex. */
    void unlock();
    
private:
    stdString name;
    size_t order;
    epicsMutex mutex;
};

// --------------------------

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
    bool add(const OrderedMutex *lock);

    /** Remove a lock from list for this thread. */
    void remove(const OrderedMutex *lock);

    /** Dump info about locks. */
    void dump() const;
    
private:
    epicsThreadId thread;
    stdList<const OrderedMutex *> locks;
    /** Check lock order. */
    bool check() const;
};

bool ThreadList::add(const OrderedMutex *lock)
{
    locks.push_back(lock);
    return check();
}

void ThreadList::remove(const OrderedMutex *lock)
{
    locks.remove(lock);
}

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
    LockMonitor();

    ~LockMonitor();
    
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

LockMonitor::LockMonitor()
{
    LOG_ASSERT(! exists);
    exists = true;
}

LockMonitor::~LockMonitor()
{
    LOG_ASSERT(exists);
    exists = false;
}

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
                fprintf(stderr, "%s (%zu): Violation of lock order\n",
                        file, line);
                dump(guard);
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

OrderedMutex::OrderedMutex(const char *name, size_t order)
    : name(name), order(order)
{
}

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

// --------------------------

TEST_CASE deadlock_test()
{
    OrderedMutex a("a", 1);
    OrderedMutex b("b", 2);
    
    try
    {
        a.lock(__FILE__, __LINE__);
        b.lock(__FILE__, __LINE__);
        
        b.unlock();
        a.unlock();
    }
    catch (GenericException &e)
    {
        FAIL("Caught exception");
    }
    try
    {
        b.lock(__FILE__, __LINE__);
        a.lock(__FILE__, __LINE__);
        FAIL("I reversed the lock order without problems?!");
        b.unlock();
        a.unlock();
    }
    catch (GenericException &e)
    {
        PASS("Caught exception:");
        printf("        %s\n", e.what());
    }
    
    TEST_OK;
}
