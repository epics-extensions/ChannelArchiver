#include "ToolsConfig.h"

const stdString::size_type stdString::npos = static_cast<size_type>(-1); 

stdString & stdString::assign(const char *s, size_type len)
{
	if (!s || !len)
	{	// assignment of NULL string
		if (_res > 0)
		{
			*_str = '\0';
			_len = 0;
		}
		return *this;
	}
	if (reserve(len))
	{
		memcpy(_str, s, len);
		_str[len] = '\0';
		_len = len;
	}
	return *this;
}

int stdString::compare(const stdString &rhs) const
{
	if (_str)
	{
		if (rhs._str)
			return strcmp(_str, rhs._str);
		// _str > NULL
		return +1;
	}
	if (rhs._str) // NULL < rhs
		return -1;
	return 0;
}

bool stdString::reserve(size_type len)
{
	if (len <= _res)
		return true;

	char *prev = _str;
	_str = new char [len+1];
	if (!_str)
	{
		_res = 0;
		_len = 0;
		return false;
	}
	_res = len;
	if (prev)
	{
		memcpy(_str, prev, _len+1);
		delete [] prev;
	}
	return true;
}

// get [from, up to n elements
stdString stdString::substr(size_type from, size_type n) const
{
	stdString s;
	
	if (from >= _len)
		return s;
	if (n == npos  ||  from+n > _len)
		n = _len - from;

	s.assign(_str + from, n);

	return s;
}         

