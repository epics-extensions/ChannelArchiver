
// Tools
#include "MsgLogger.h"
#include "GenericException.h"
#include "UnitTest.h"

class CPElement
{
public:
    CPElement(void *item) : item(item), next(0) {}

    void *getItem() const                       { return item; }

    CPElement *getNext() const                  { return next; }
    
    void setNext(CPElement *e)                  { next = e; }
    
private:
    void       *item;
    CPElement *next;
};

/** A List that allows concurrent access.
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
    ConcurrentPtrList();

    ~ConcurrentPtrList();
    
    void add(void *i);
    
    class ConcurrentPtrListIterator iterator();
    
private:
    CPElement *list;
};

/** Iterator for ConcurrentPtrList.
 *  @see ConcurrentPtrList
 */
class ConcurrentPtrListIterator
{
public:
    ConcurrentPtrListIterator(CPElement *element);
    
    bool hasNext();
    
    void *next();
private:
    CPElement *element;
};
 
// ConcurrentPtrList --
ConcurrentPtrList::ConcurrentPtrList()
    : list(0)
{
}

ConcurrentPtrList::~ConcurrentPtrList()
{
    CPElement *e = list;
    while (e)
    {
        CPElement *del = e;
        e = e->getNext();
        delete del;
    }
}

void ConcurrentPtrList::add(void *i)
{
    CPElement *n = new CPElement(i);
    if (list == 0)
    {   // list was empty
        list = n;
        return;
    }
    // Loop to last element
    CPElement *e = list;
    while (e->getNext())
        e = e->getNext();
    // make it point to the new element
    e->setNext(n);
}

ConcurrentPtrListIterator ConcurrentPtrList::iterator()
{
    return ConcurrentPtrListIterator(list);
}

// ConcurrentPtrListIterator --
ConcurrentPtrListIterator::ConcurrentPtrListIterator(CPElement *element)
    : element(element)
{
}

bool ConcurrentPtrListIterator::hasNext()
{
    return element != 0;
}

void *ConcurrentPtrListIterator::next()
{
    if (!element)
        throw GenericException(__FILE__, __LINE__, "End of list");
    void *item = element->getItem();
    element = element->getNext();
    return item;
}

/** Type-save wrapper for ConcurrentPtrListIterator.
 *  @see ConcurrentPtrListIterator
 */
template<class T> class ConcurrentListIterator
{
public:
    ConcurrentListIterator(ConcurrentPtrListIterator iter) : iter(iter) {}
    bool hasNext()  { return iter.hasNext(); }
    T *next()       { return (T *) iter.next(); }
private:
    ConcurrentPtrListIterator iter;
};

/** Type-save wrapper for ConcurrentPtrList.
 *  @see ConcurrentPtrList
 */
template<class T> class ConcurrentList : private ConcurrentPtrList
{
public:  
    void add(T *i)
    {
        ConcurrentPtrList::add(i);
    }
    
    ConcurrentListIterator<T> iterator()
    {
        return ConcurrentListIterator<T>(ConcurrentPtrList::iterator());
    }
};

TEST_CASE test_concurrent_list()
{
    stdString fred("fred");
    stdString freddy("freddy");
    stdString jane("jane");
    stdString janet("janet");
    
    ConcurrentList<stdString> subscribers;
    //ConcurrentPtrList subscribers;
    subscribers.add(&fred);
    subscribers.add(&freddy);
    subscribers.add(&jane);
    subscribers.add(&janet);
    
    ConcurrentListIterator<stdString> s = subscribers.iterator();
    //ConcurrentPtrListIterator s = subscribers.iterator();
    while (s.hasNext())
    {
        stdString *name = s.next();
        puts(name->c_str());
    }
    
    TEST_OK;
}

TEST_CASE test_list()
{
    printf("------------------------------------------\n");
    printf("List Test\n");
    printf("------------------------------------------\n");

    stdList<stdString> list;
    
    list.push_back(stdString("A"));
    list.push_back(stdString("B"));
    list.push_back(stdString("C"));

    printf("Dump\n");
    TEST(list.size() == 3);
    stdList<stdString>::iterator i;
    for (i = list.begin(); i!=list.end(); ++i)
    {
        printf("        %s\n", i->c_str());
    }
    
    printf("Append while traversing\n");
    i = list.begin();
    printf("        %s\n", i->c_str());
    ++i;
    list.push_back(stdString("D added while traversing"));
    for (/**/; i!=list.end(); ++i)
    {
        printf("        %s\n", i->c_str());
    }

#if 0
    printf("Delete 'A' while traversing\n");
    i = list.begin();
    // This results in crash, since we remove an element
    list.pop_front();
    // ... and then try to access the deleted element:
    printf("        %s\n", i->c_str());
    ++i;
    for (/**/; i!=list.end(); ++i)
    {
        printf("        %s\n", i->c_str());
    }
#endif

    TEST_OK;
}

