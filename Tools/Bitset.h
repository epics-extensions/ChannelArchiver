#ifndef __BITSET_H__
#define __BITSET_H__

#include <MsgLogger.h>

//CLASS BitSet
//
// Inspired by STL bitset.
//
// Reasons for reinventing this type of class:
// <UL>
// <LI>The STL version is not part of e.g. the egcs compiler (as of Sept. 1999).
// <LI>This BitSet grows during runtime, size isn't had-coded at compiletime.
// </UL>
//
class BitSet
{
public:
	BitSet ();
	~BitSet ();

	BitSet & operator = (const BitSet &rhs);

	//* Set/clear bits
	void set (size_t bit);
	void clear (size_t bit);
	void set (size_t bit, bool value);

	//* Check individual bit.
	//
	// Valid bit numbers are [0...size()[.
	bool operator [] (size_t bit) const;

	//* Whole bitset empty?
	bool empty () const;

	//* Any bit set?
	bool any () const;

	//* Number of bits set
	size_t count () const;

	//* Number of bits in Bitset
	size_t size () const;

	//* Return 001000.... type of string
	stdString to_string () const;

	//* Grow so that size is at least minumum
	void grow (size_t minimum);

private:
	BitSet (const BitSet &rhs); // not implemented

	typedef unsigned long W32;
	W32 *_bits;
	size_t _num;	// # of W32 that _bits points to
	size_t _size;	// bits used
};

inline size_t BitSet::size () const
{	return _size;	}

inline void BitSet::set (size_t bit)
{
	LOG_ASSERT (bit < size());
	W32 *b = _bits + (bit / 32);
	bit %= 32;
	*b |= (1 << bit);
}

inline void BitSet::clear (size_t bit)
{
	LOG_ASSERT (bit < size());
	W32 *b = _bits + (bit / 32);
	bit %= 32;
	*b &= ~(1 << bit);
}

inline void BitSet::set (size_t bit, bool value)
{
	if (value)
		set (bit);
	else
		clear (bit);
}

inline bool BitSet::operator [] (size_t bit) const
{
	if (bit >= size())
		return false;
	W32 *b = _bits + (bit / 32);
	bit %= 32;
	return !!(*b & (1 << bit));
}

inline size_t BitSet::count () const
{
	size_t c = 0;
	W32 t, *b=_bits;
	for (size_t num=_num; num>0; --num)
	{
		for (t=*b; t; t >>= 4) // hack taken from STL
			c += "\0\1\1\2\1\2\2\3\1\2\2\3\2\3\3\4" [t & 0xF];
		++b;
	}
	return c;
}

inline bool BitSet::any () const
{
	W32 *b;
	size_t num;
	for (b=_bits, num=_num; num>0; --num)
	{
		if (*b)
			return true;
		++b;
	}
	return false;
}

inline bool BitSet::empty () const
{	return ! any();	}

#endif //__BITSET_H__
