#ifndef __CIRCULARBUFFER_H__
#define __CIRCULARBUFFER_H__

#include<ValueI.h>
#include<epicsMutex.h>

// Circular buffer:
// Each channel has one to buffer the incoming values
// until they are written to the disk.
//
// Hopefully thread-save.
//
class CircularBuffer
{
public:
    CircularBuffer();
    ~CircularBuffer();

    // Keep memory as is, but reset to have 0 entries
    void reset();
    
    CircularBuffer & operator = (const CircularBuffer &);

    void allocate(DbrType type, DbrCount count, double scan_period);

    // Capacity of this CircularBuffer (Number of elements)
    size_t getSize()
    {   return _num; }

    size_t getOverwrites()
    {   return _overwrites; }

    void resetOverwrites()
    {   _lock.lock(); _overwrites = 0; _lock.unlock();    }

    void addRawValue(const RawValue::Data *raw_value);

    const RawValue::Data *removeRawValue();

    size_t getCount();

private:
    CircularBuffer(const CircularBuffer &); // not impl.

    void allocate(DbrType type, DbrCount count, size_t num);
    RawValue::Data *getElement(RawValue::Data *buf, size_t i);
    RawValue::Data *getNextElement();

    epicsMutex      _lock;
    DbrType         _type;
    DbrCount        _count;
    RawValue::Data  *_buffer;
    size_t          _element_size;// == RawValue::getSize (_type, _count)
    size_t          _num;         // number of elements in buffer
    size_t          _head;        // index of current element
    size_t          _tail;        // before! last element, _tail==_head: empty
    size_t          _overwrites;
};

inline RawValue::Data *CircularBuffer::getElement(RawValue::Data *buf,
                                                   size_t i)
{   return (RawValue::Data *) (((char *)buf) + i * _element_size); }

inline void CircularBuffer::addRawValue(const RawValue::Data *raw_value)
{
    _lock.lock();
    memcpy(getNextElement(), raw_value, _element_size);
    _lock.unlock();
}

inline size_t CircularBuffer::getCount()
{
    size_t count;
    _lock.lock();
    if (_head >= _tail)
        count = _head - _tail;
    else    
        //     #(tail .. end)      + #(start .. head)
        count = (_num - _tail - 1) + (_head + 1);
    _lock.unlock();

    return count;
}

inline const RawValue::Data *CircularBuffer::removeRawValue()
{
    RawValue::Data *val;

    _lock.lock();
    if (_tail == _head)
        val = 0;
    else
    {
        if (++_tail >= _num)
            _tail = 0;

        val = getElement(_buffer, _tail);
    }
    _lock.unlock();
    return val;
}

#endif //__CIRCULARBUFFER_H__

