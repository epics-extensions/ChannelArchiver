#ifdef CIRCBUF_TEST
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef short DbrType;
typedef short DbrCount;

class RawValue
{
public:
    typedef long Data;
    static Data *allocate(DbrType, DbrCount, int num)
    {    return (Data *)malloc(sizeof(long)*num); }
    static void free(Data *data)
    {    ::free(data); }
    static size_t getSize(DbrType t, DbrCount c)
    {    return sizeof(long); }
};

#include "CircularBuffer.h"

int main()
{
    CircularBuffer buffer;
    long i;

    buffer.allocate(0, 0, 8);
    puts("--- Empty");
    printf("Capacity: %d\n", buffer.getCapacity());
    printf("Count   : %d\n", buffer.getCount());
    puts("--- Adding 5 values");
    for (i=0; i<5; ++i)
        buffer.addRawValue(&i);
    printf("Count   : %d\n", buffer.getCount());

    const long *p;
    puts("--- Peeking the values out");
    for (i=0; i<6; ++i)
    {
        p = buffer.getRawValue(i);
        if (p)
            printf("Value: %ld\n", *p);
        else
            printf("Value: none\n");
    }
    puts("--- Removing the values");
    while (p=buffer.removeRawValue())
        printf("Value: %ld\n", *p);
    printf("Count   : %d\n", buffer.getCount());
    printf("Overwrites: %d \n", buffer.getOverwrites());

    puts("--- Generating overwrites");
    for (i=0; i<2*buffer.getCapacity(); ++i)
        buffer.addRawValue(&i);
    printf("Overwrites: %d \n", buffer.getOverwrites());
    buffer.resetOverwrites();
    printf("Overwrites: %d \n", buffer.getOverwrites());
    
    puts("--- Removing the values");
    while (p=buffer.removeRawValue())
        printf("Value: %ld\n", *p);
    printf("Count   : %d\n", buffer.getCount());
    
    return 0;
}

#else

#include "MsgLogger.h"
#include "Engine.h"
#include "CircularBuffer.h"

#endif


// The Circular buffer implementation:
//
// Initial: tail = head = 0.
//
// valid entries:
// tail+1, tail+2 .... head

CircularBuffer::CircularBuffer()
{
	type = 0;
	count = 0;
	buffer = 0;
	element_size = 0;
	max_index = 0;
	head = 0;
	tail = 0;
	overwrites = 0;
}

CircularBuffer::~CircularBuffer()
{
    RawValue::free(buffer);
}

void CircularBuffer::allocate(DbrType type, DbrCount count, size_t num)
{
    if (this->type==type && this->count==count && max_index > num)
        // can hold that already
        return;
    
    // Since head == tail indicates empty, we can only
    // hold max_index-1 elements. Inc num by one to account for that:
    ++num;
	if (buffer)
		RawValue::free(buffer);
	buffer = RawValue::allocate(type, count, num);
	element_size = RawValue::getSize(type, count);
	this->type = type;
	this->count = count;
	max_index = num;
}

size_t CircularBuffer::getCount()
{
    size_t count;
    if (head >= tail)
        count = head - tail;
    else    
        //     #(tail .. end)      + #(start .. head)
        count = (max_index - tail - 1) + (head + 1);
    return count;
}

void CircularBuffer::reset()
{
	head = 0;
	tail = 0;
	overwrites = 0;
}

const RawValue::Data *CircularBuffer::getRawValue(size_t i)
{
    if (i<0 || i >= getCount())
        return 0;
    i = (tail + 1 + i) % max_index;
    return getElement(i);
}

const RawValue::Data *CircularBuffer::removeRawValue()
{
    RawValue::Data *val;

    if (tail == head)
        val = 0;
    else
    {
        if (++tail >= max_index)
            tail = 0;

        val = getElement(tail);
    }
    return val;
}

RawValue::Data *CircularBuffer::getNextElement()
{
	// compute the place in the circular queue
	if (++head >= max_index)
		head = 0;

	// here is the over write of the queue
	// we slow it down in the event handler
	// we reset it in the archiving routine
	if (head == tail)
	{
		++overwrites;
		if (++tail >= max_index)
			tail = 0;
	}
	return getElement(head);
}

RawValue::Data *CircularBuffer::getElement(size_t i)
{   return (RawValue::Data *) (((char *)buffer) + i * element_size); }

