#ifndef __CIRCULARBUFFER_H__
#define __CIRCULARBUFFER_H__

#include <ValueI.h>
#include <Thread.h>

BEGIN_NAMESPACE_CHANARCH

// Circular buffer:
// Each channel has one to buffer the incoming values
// until they are written to the disk.
//
// Hopefully thread-save.
//
class CircularBuffer
{
public:
	CircularBuffer ();
	~CircularBuffer ();

	CircularBuffer & operator = (const CircularBuffer &);

	void allocate (DbrType type, DbrCount count, double scan_period);

	// Capacity of this CircularBuffer (Number of elements)
	size_t getSize ()
	{	return _num; }

	size_t getOverWrites ()
	{	return _overwrites; }

	void reset ()
	{	_lock.take(); _overwrites = 0; _lock.give();	}

	void addRawValue (const RawValueI::Type *raw_value);

	const RawValueI::Type *removeRawValue ();

	size_t getCount ();

private:
	CircularBuffer (const CircularBuffer &); // not impl.

	void allocate (DbrType type, DbrCount count, size_t num);
	RawValueI::Type *getElement (RawValueI::Type *buf, size_t i);
	RawValueI::Type *getNextElement ();

	ThreadSemaphore	_lock;

	DbrType			_type;
	DbrCount		_count;
	RawValueI::Type	*_buffer;
	size_t			_element_size;	// == RawValue::getSize (_type, _count)
	size_t			_num;			// number of elements in buffer
	size_t			_head;			// index of current element
	size_t			_tail;			// _before_ last element,   _tail==_head: empty
	size_t			_overwrites;
};

inline RawValueI::Type *CircularBuffer::getElement (RawValueI::Type *buf, size_t i)
{	return (RawValueI::Type *) (((char *)buf) + i * _element_size);	}

inline void CircularBuffer::addRawValue (const RawValueI::Type *raw_value)
{	
	_lock.take ();
	memcpy (getNextElement(), raw_value, _element_size);
	_lock.give ();
}

inline size_t CircularBuffer::getCount ()
{
	size_t count;
	_lock.take ();
	if (_head >= _tail)
		count = _head - _tail;
	else	
		//     #(tail .. end)     + #(start .. head)
		count = (_num - _tail - 1) + (_head + 1);
	_lock.give ();

	return count;
}

inline const RawValueI::Type *CircularBuffer::removeRawValue ()
{
	RawValueI::Type *val;

	_lock.take ();
	if (_tail == _head)
		val = 0;
	else
	{
		if (++_tail >= _num)
			_tail = 0;

		val = getElement (_buffer, _tail);
	}
	_lock.give();
	return val;
}

END_NAMESPACE_CHANARCH

#endif //__CIRCULARBUFFER_H__
