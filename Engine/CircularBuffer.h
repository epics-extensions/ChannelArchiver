// -*- c++ -*-
#ifndef __CIRCULARBUFFER_H__
#define __CIRCULARBUFFER_H__

#ifdef CIRCBUF_TEST
#include <stdlib.h>
#else
#include <epicsMutex.h>
#include <RawValue.h>
#endif

/// Circular buffer:
/// Each ArchiveChannel has one to buffer the incoming values
/// of type dbr_time_xxx until they are written to the disk.
class CircularBuffer
{
public:
    CircularBuffer();
    ~CircularBuffer();
 
    /// Allocate buffer for num*(type,count) values.
    void allocate(DbrType type, DbrCount count, size_t num);

    /// Capacity, that is: max. number of elements
    size_t getCapacity()
    {   return max_index-1; }

    /// Number of values in the buffer, 0...(capacity-1)
    size_t getCount();

    /// Keep memory as is, but reset to have 0 entries
    void reset();
    
    /// Number of samples that had to be dropped
    size_t getOverwrites()
    {   return overwrites; }

    /// Doesn't change buffer at all, just reset the overwrite count
    void resetOverwrites()
    {   overwrites = 0; }

    /// Advance pointer to the next element and return it.

    /// This allows you to fiddle with that element yourself,
    /// otherwise see addRawValue.
    ///
    RawValue::Data *getNextElement();
    
    /// Copy a raw value into the buffer
    void addRawValue(const RawValue::Data *raw_value)
    {   memcpy(getNextElement(), raw_value, element_size); }

    /// Get a pointer to value number i without removing it

    ///
    /// Returns 0 if i is invalid.
    ///
    const RawValue::Data *getRawValue(size_t i);
    
    /// Return pointer to the oldest value in the buffer and remove it.

    ///
    /// Returns 0 of there's nothing more to remove.
    ///
    const RawValue::Data *removeRawValue();

private:
    CircularBuffer(const CircularBuffer &); // not impl.
    CircularBuffer & operator = (const CircularBuffer &); // not impl.

    RawValue::Data *getElement(size_t i);

    DbrType         type;        // dbr_time_xx
    DbrCount        count;       // array size of type
    RawValue::Data  *buffer;     // the buffer
    size_t          element_size;// == RawValue::getSize (_type, _count)
    size_t          max_index;   // max. number of elements in buffer
    size_t          head;        // index of current element
    size_t          tail;        // before(!) last element, _tail==_head: empty
    size_t          overwrites;  // # of elements we had to drop
};

#endif //__CIRCULARBUFFER_H__

