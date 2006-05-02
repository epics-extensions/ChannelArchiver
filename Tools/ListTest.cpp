
// Tools
#include "MsgLogger.h"
#include "GenericException.h"
#include "UnitTest.h"

class CPElement
{
public:
    CPElement(void *item) : item(item), next(0) {}

    void *getItem() const                       { return item; }
    
    void setItem(void *i)                       { item = i; }

    CPElement *getNext() const                  { return next; }
    
    void setNext(CPElement *e)                  { next = e; }
    
private:
    void       *item;
    CPElement  *next;
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
    
    void add(void *item);

    void remove(void *item);
    
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
    CPElement *next_element;
    void      *item;
    void getNext();
};
 
// ConcurrentPtrList --

// Implementation:
// A simple linked list,
// where items with value '0' are indicators for deleted elements.
// They are skipped by any (ongoing) iterator,
// and removed later.
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

void ConcurrentPtrList::add(void *item)
{
    if (list == 0)
    {   // list was empty
        list = new CPElement(item);
        return;
    }
    // Loop to last element
    CPElement *e = list;
    while (1)
    {
        if (e->getItem() == 0)
        {   // Re-use list item that had been removed.
            e->setItem(item);
            return;
        }
        if (e->getNext() == 0)
        {   // Reached end of list, append new element.
            e->setNext(new CPElement(item));
            return;
        }
        e = e->getNext();
    }
}

void ConcurrentPtrList::remove(void *item)
{
    CPElement *e = list;
    while (e)
    {
        if (e->getItem() == item)
        {
            e->setItem(0);
            return;
        }
        e = e->getNext();
    }
    throw GenericException(__FILE__, __LINE__, "Unknown item");    
}

ConcurrentPtrListIterator ConcurrentPtrList::iterator()
{
    return ConcurrentPtrListIterator(list);
}

// ConcurrentPtrListIterator --

// Idea: item is the item that next() will return.
//       It was copied from the 'current' list element
//       in case somebody modifies the list between
//       calls to hasNext() and next().
//       The next_element already points to the
//       following list element.
ConcurrentPtrListIterator::ConcurrentPtrListIterator(CPElement *element)
    : next_element(element), item(0)
{
    getNext();
}

void ConcurrentPtrListIterator::getNext()
{
    if (next_element == 0)
    {
        item = 0;
        return;
    }
    do
    {
        item = next_element->getItem();
        next_element = next_element->getNext();
    }
    while (item == 0  &&  next_element);
}

bool ConcurrentPtrListIterator::hasNext()
{
    return item != 0;
}

void *ConcurrentPtrListIterator::next()
{
    if (!item)
        throw GenericException(__FILE__, __LINE__, "End of list");
    void *result = item;
    getNext();
    return result;
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
    
    void remove(T *i)
    {
        ConcurrentPtrList::remove(i);
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
    stdString bob("bob");
    
    ConcurrentList<stdString> subscribers;
    subscribers.add(&fred);
    subscribers.add(&freddy);
    subscribers.add(&jane);
    subscribers.add(&janet);
    
    COMMENT("Simple Iteration");
    ConcurrentListIterator<stdString> s = subscribers.iterator();
    TEST(s.hasNext());
    TEST(*s.next() == "fred");
    TEST(s.hasNext());
    TEST(*s.next() == "freddy");
    TEST(s.hasNext());
    TEST(*s.next() == "jane");
    TEST(s.hasNext());
    TEST(*s.next() == "janet");
    TEST(s.hasNext() == false);
    try
    {
        s.next();
        FAIL("Allowed to step past last element?!");
    }
    catch (GenericException &e)
    {
        PASS(e.what());
    }
        
    COMMENT("Iteration where 'fred' is removed while iterator is on it");
    // Start over: Position on first entry, "fred"
    s = subscribers.iterator();
    TEST(s.hasNext());
    // Remove element, ...
    subscribers.remove(&fred);
    // but iterator was already on the element, so you still get it:
    TEST(*s.next() == "fred");
    TEST(s.hasNext());
    TEST(*s.next() == "freddy");
    TEST(s.hasNext());
    TEST(*s.next() == "jane");
    TEST(s.hasNext());
    TEST(*s.next() == "janet");
    TEST(s.hasNext() == false);

    COMMENT("Iteration where 'fred' is gone, but 'bob' was added.");
    COMMENT("Then add 'fred' again while iterating.");
    subscribers.add(&bob);
    s = subscribers.iterator();
    TEST(s.hasNext());
    TEST(*s.next() == "bob");
    TEST(s.hasNext());
    TEST(*s.next() == "freddy");
    subscribers.add(&fred);
    TEST(s.hasNext());
    TEST(*s.next() == "jane");
    TEST(s.hasNext());
    TEST(*s.next() == "janet");
    TEST(s.hasNext());
    TEST(*s.next() == "fred");
    TEST(s.hasNext() == false);
    
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

