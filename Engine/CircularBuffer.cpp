// Tools
#include <MsgLogger.h>
#include <epicsTimeHelper.h>
// Engine
#include "Engine.h"
#include "CircularBuffer.h"

#undef DEBUG_CIRCBUF

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
    if (buffer && this->type==type && this->count==count && max_index > num)
        return; // can hold that already
    reset();
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

size_t CircularBuffer::getCount() const
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

void CircularBuffer::addRawValue(const RawValue::Data *value)
{
#ifdef DEBUG_CIRCBUF
    printf("CircularBuffer(0x%lX)::add: ", (unsigned long)this);
    RawValue::show(stdout, type, count, value); 
#endif
    if (!isValidTime(RawValue::getTime(value)))
    {
        LOG_MSG("CircularBuffer added bad time stamp!\n");
    }
    memcpy(getNextElement(), value, element_size);
}

const RawValue::Data *CircularBuffer::getRawValue(size_t i) const
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
    if (val && !isValidTime(RawValue::getTime(val)))
    {
        LOG_MSG("CircularBuffer returns bad time stamp!\n");
    }

#ifdef DEBUG_CIRCBUF
    printf("CircularBuffer(0x%lX)::remove: ", (unsigned long)this);
    RawValue::show(stdout, type, count, val); 
#endif
    return val;
}

void CircularBuffer::dump() const
{
    size_t i, num = getCount();
    printf("CircularBuffer type %d, count %d, capacity %d:\n",
           (int)type, (int)count, (int)getCapacity());
    for (i=0; i<num; ++i)
    {
        printf("#%3d: ", i);
        RawValue::show(stdout, type, count, getRawValue(i));
    }
}

RawValue::Data *CircularBuffer::getElement(size_t i) const
{
    return (RawValue::Data *) (((char *)buffer) + i * element_size);
}

#ifdef CIRCBUF_TEST

/* For standalone test, compile like this:

g++ -g -D CIRCBUF_TEST -o CircularBuffer CircularBuffer.cpp \
    -I ../Tools -I ../Storage \
    -I $EPICS_BASE_RELEASE/include \
    -I $EPICS_BASE_RELEASE/include/os/$HOST_ARCH \
    ../Storage/O.$EPICS_HOST_ARCH/libStorage.a \
    ../Tools/O.$EPICS_HOST_ARCH/libTools.a \
    -L $EPICS_BASE_RELEASE/lib/$EPICS_HOST_ARCH \
    -l db -l ca -l Com
*/

int main()
{
    DbrType type = DBR_TIME_DOUBLE;
    DbrCount count = 1;
    CircularBuffer buffer;
    long N, i;

    buffer.allocate(type, count, 8);
    puts("--- Empty");
    printf("Capacity: %d\n", buffer.getCapacity());
    printf("Count   : %d\n", buffer.getCount());

    RawValue::Data *data = RawValue::allocate(type, count, 1);
    RawValue::setStatus(data, 0, 0);

    N = 2;
    printf("Adding %d values\n", N);
    for (i=0; i<N; ++i)
    {
        RawValue::setTime(data, epicsTime::getCurrent());
        data->value = 3.14 + i;
        buffer.addRawValue(data);
    }
    printf("Capacity: %d\n", buffer.getCapacity());
    printf("Count   : %d\n", buffer.getCount());

    const RawValue::Data *p;
    while (p=buffer.removeRawValue())
        RawValue::show(stdout, type, count, p);
    
    N = 20;
    printf("Adding %d values\n", N);
    for (i=0; i<N; ++i)
    {
        RawValue::setTime(data, epicsTime::getCurrent());
        data->value = 3.14 + i;
        buffer.addRawValue(data);
    }
    puts("--- Peeking the values out");
    for (i=0; i<buffer.getCount(); ++i)
        RawValue::show(stdout, type, count, buffer.getRawValue(i));
    
    puts("--- Removing the values");
    while (p=buffer.removeRawValue())
        RawValue::show(stdout, type, count, p);

    RawValue::free(data);
    
    return 0;
}

#endif
