#ifndef __CONCURRENTLIST_H__
#define __CONCURRENTLIST_H__

// Tools
#include <Guard.h>

/** \ingroup Tools
 *  A List that allows concurrent access.
 *  <p>
 *  One can add or remove elements while
 *  someone else is traversing the list.
 *  Of course the list is locked in add(),
 *  next(), .., but not for the full duration
 *  of traversal.
 */
class ConcurrentPtrList
{
public:
    /** Construct a new list. */
    ConcurrentPtrList();

    /** Delete list and all its elements. */
    ~ConcurrentPtrList();
    
    /** @return Returns the mutex for the Guard passed to other methods. */
    epicsMutex &getMutex() { return mutex; }
    
    /** Add an item to the list.
     *  <p>
     *  The position of that item on the list is
     *  not necessarily predicatable.
     *  Typically at the end, but may be at a 'reused' location.
     */
    void add(epicsMutexGuard &guard, void *item);

    /** Remove an item from the list. */
    void remove(epicsMutexGuard &guard, void *item);
    
    /** Obtain iterator, positioned at the start of the list. */
    class ConcurrentPtrListIterator iterator(epicsMutexGuard &guard);
    
private:
    epicsMutex       mutex;
    class CPElement *list;
};

/** \ingroup Tools
 *  Iterator for ConcurrentPtrList.
 *  @see ConcurrentPtrList
 */
class ConcurrentPtrListIterator
{
public:
    /** Constructor. Users should use ConcurrentPtrList::iterator().
     *  @see ConcurrentPtrList::iterator()
     */
    ConcurrentPtrListIterator(epicsMutexGuard &guard,
                              ConcurrentPtrList *list,
                              class CPElement *element);

    /** Destructor. */
    ~ConcurrentPtrListIterator();

    /** @return Returns the mutex for the Guard passed to other methods. */
    epicsMutex &getMutex() { return list->getMutex(); }
    
    /** @return Returns true if there is another element. */
    bool hasNext(epicsMutexGuard &guard);
    
    /** @return Returns the next element. */
    void *next(epicsMutexGuard &guard);
private:
    ConcurrentPtrList *list;
    class CPElement   *next_element;
    void              *item;

    void getNext(epicsMutexGuard &guard);
};
 
/** \ingroup Tools
 *  Type-save wrapper for ConcurrentPtrListIterator.
 *  @see ConcurrentPtrListIterator
 */
template<class T> class ConcurrentListIterator
{
public:
    ConcurrentListIterator(ConcurrentPtrListIterator iter) : iter(iter) {}
    
    /** @see ConcurrentPtrListIterator::hasNext */
    bool hasNext()
    {
        epicsMutexGuard guard(iter.getMutex());
        return iter.hasNext(guard);
    }
    
    /** @see ConcurrentPtrListIterator::next */
    T *next()
    {
        epicsMutexGuard guard(iter.getMutex());
        return (T *) iter.next(guard);
    }
private:
    ConcurrentPtrListIterator iter;
};

/** \ingroup Tools
 *  Type-save wrapper for ConcurrentPtrList.
 *  @see ConcurrentPtrList
 */
template<class T> class ConcurrentList : private ConcurrentPtrList
{
public:  
    /** @see ConcurrentPtrList::add */
    void add(T *i)
    {
        epicsMutexGuard guard(ConcurrentPtrList::getMutex());
        ConcurrentPtrList::add(guard, i);
    }
    
    /** @see ConcurrentPtrList::remove */
    void remove(T *i)
    {
        epicsMutexGuard guard(ConcurrentPtrList::getMutex());
        ConcurrentPtrList::remove(guard, i);
    }

    /** @see ConcurrentPtrList::iterator */
    ConcurrentListIterator<T> iterator()
    {
        epicsMutexGuard guard(ConcurrentPtrList::getMutex());
        return ConcurrentListIterator<T>(ConcurrentPtrList::iterator(guard));
    }
};

#endif 
