#include "CircularBuffer.h"
#include "MsgLogger.h"
#include "Engine.h"

// Initial: tail = head = 0.
//
// valid entries:
// tail+1, tail+2 .... head

CircularBuffer::CircularBuffer()
{
	_type = 0;
	_count = 0;
	_buffer = 0;
	_element_size = 0;
	_num = 0;
	_head = 0;
	_tail = 0;
	_overwrites = 0;
}

CircularBuffer::~CircularBuffer()
{
	RawValueI::free(_buffer);
}

void CircularBuffer::reset()
{
    _lock.lock();
	_head = 0;
	_tail = 0;
	_overwrites = 0;
    _lock.unlock();
}

CircularBuffer & CircularBuffer::operator = (const CircularBuffer &rhs)
{
	_type = rhs._type;
	_count = rhs._count;
	_element_size = rhs._element_size;
	_num = rhs._num;
	_head = rhs._head;
	_tail = rhs._tail;
	_overwrites = rhs._overwrites;

	_buffer = 0;

	if (rhs._buffer)
		allocate(_type, _count, _num);
	return *this;
}

RawValueI::Type *CircularBuffer::getNextElement()
{
	// compute the place in the circular queue
	if (++_head >= _num)
		_head = 0;

	// here is the over write of the queue
	// we slow it down in the event handler
	// we reset it in the archiving routine
	if (_head == _tail)
	{
		++_overwrites;
		if (++_tail >= _num)
			_tail = 0;
	}
	return getElement(_buffer, _head);
}

void CircularBuffer::allocate(DbrType type, DbrCount count, double scan_period)
{
	size_t	num;

	double write_period = theEngine->getWritePeriod();
	if (write_period <= 0)
		num = 100;
	else
		num = size_t(write_period * theEngine->getBufferReserve()
                     / scan_period);
	if (num < 3)
		num = 3;

	allocate(type, count, num);
}

void CircularBuffer::allocate(DbrType type, DbrCount count, size_t num)
{
	RawValueI::Type *buffer;
	if (_type==type && _count==count && _num >= num) // can hold that already
		return;

	_lock.lock();
	buffer = RawValueI::allocate(type, count, num);
	if (_type!=type && _count!=count && _buffer) // old buffer, diff. type?
	{
		RawValueI::free(_buffer);
		_buffer = 0;
	}

	_element_size = RawValueI::getSize(type, count);
	_type = type;
	_count = count;

	if (_buffer) // old (smaller) buffer to copy in?
	{
		// buffer is bigger than old _buffer
		if (_tail < _head)
		{
			memcpy(getElement(buffer, 1),
                   getElement(_buffer, _tail+1), _head-_tail);
			_head -= _tail;
			_tail = 0;
		}
		else
		if (_tail > _head)
		{
			size_t tail_elems = _num - _tail - 1;
			memcpy(getElement(buffer, 1),
                   getElement(_buffer, _tail+1), tail_elems);
			memcpy(getElement(buffer, tail_elems), _buffer, _head+1);
			_tail = 0;
			_head += tail_elems + 1;
		}
		// else: _head == _tail, empty _buffer
		RawValueI::free(_buffer);
	}

	_num = num;
	_buffer = buffer;
	_lock.unlock();
}

