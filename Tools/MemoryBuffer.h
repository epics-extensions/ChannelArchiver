#ifndef __MEMORYBUFFER_H__
#define __MEMORYBUFFER_H__

#include <ToolsConfig.h>

// MemoryBuffer<T>:
// Can be used like pointer to T
// which has reserved size,
// may grow in size (new, no realloc)
// and is deallocated automatically.
template <class T>
class MemoryBuffer
{
public:
	MemoryBuffer () : _mem(0), _size(0)	{}

	~MemoryBuffer ()
	{
		if (_mem)
			delete [] _mem;
	}

	// Reserve memory space, may grow current buffer
	void reserve (size_t wanted)
	{
		if (_size < wanted)
		{
			if (_mem)
				delete [] _mem;
			_mem = new char [wanted];
			_size = wanted;
		}
	}

	// Access as (T *)
	const T *mem () const
	{	return (const T *)_mem; }
	T *mem ()
	{	return (T *)      _mem; }

	size_t getBufferSize () const
	{	return _size; }

private:
	char	*_mem;
	size_t	_size;
};

#endif //__MEMORYBUFFER_H__
