#ifndef __MEMORYBUFFER_H__
#define __MEMORYBUFFER_H__

#include <ToolsConfig.h>

/// \ingroup Tools A memory region that can be resized.

/// A MemoryBuffer<T> which has reserved size,
/// may grow in size (new, no realloc)
/// and is automatically deallocated.
template <class T>
class MemoryBuffer
{
public:
    /// Constructor: Buffer is initially empty.
	MemoryBuffer() : memory(0), size(0)	{}

    /// Desstructor.
	~MemoryBuffer()
	{
		if (memory)
			delete [] memory;
	}

	/// Reserve or grow buffer.
	void reserve(size_t wanted)
	{
		if (size < wanted)
		{
			if (memory)
				delete [] memory;
			memory = new char [wanted];
			size = wanted;
		}
	}

	/// Access as (T *)
	const T *mem () const
	{	return (const T *)memory; }

	/// Access as (T *)
	T *mem ()
	{	return (T *) memory; }

    /// Get current size.
	size_t getBufferSize () const
	{	return size; }

private:
	char	*memory;
	size_t	size;
};

#endif //__MEMORYBUFFER_H__
