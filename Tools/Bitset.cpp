#include <Bitset.h>

BitSet::BitSet ()
{
	_bits = 0;
	_size = 0;
	_num = 0;
}

BitSet::~BitSet ()
{
	delete [] _bits;
}

BitSet & BitSet::operator = (const BitSet &rhs)
{
	_num = rhs._num;
	_size = rhs._size;
	if (_num > 0)
	{
		_bits = new W32[_num];
		memcpy (_bits, rhs._bits, _num*4);
	}
	else
		_bits = 0;
	return *this;
}

void BitSet::grow (size_t minimum)
{
	if (_size >= minimum)
		return;

	if (! minimum)
		return;
	_num = (minimum-1) / 32 + 1;
	W32 *bits = new W32[_num];
	memset (bits, 0, _num*4);

	if (_bits)
	{
		memcpy (bits, _bits, _size / 8);
		delete [] _bits;
	}
	_bits = bits;
	_size = _num * 32;
}

// Excludes leading '0's...
stdString BitSet::to_string () const
{
	bool leading = false;
	size_t i=size();
	stdString s;
	s.reserve (i);
	while (i > 0)
	{
		--i;
		if (operator[](i))
		{
			s += '1';
			leading = true;
		}
		else
			if (leading || i==0)
				s += '0';
	}

	return s;
}


