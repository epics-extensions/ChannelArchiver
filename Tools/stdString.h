#ifndef __STD_STRING_H__
#define __STD_STRING_H__

#include <cstring>

/// Meant to behave like std::string,
/// was introduced when egcs implementation lead
/// to memory leak - and then kept.
class stdString
{
public:
	typedef size_t size_type;

	/// Create/delete string
	stdString();
	stdString(const stdString &rhs);
	stdString(const char *s);
	virtual ~stdString();

	/// Length information
	size_type length() const;
	bool empty() const;

	/// Get C-type data which is always != NULL
	const char *c_str() const;

	/// Character access, <I>no test for valid index value!!</I>
	char operator [] (size_t index) const;

	/// Assignments
	///
	/// (assign (0, 0) implemented as means of "clear()")
	stdString & assign(const char *s, size_type len);
	stdString & operator = (const stdString &rhs);
	stdString & operator = (const char *rhs);

	/// Concatenations
	/// (prefer reserve() && += to + for performance)
	stdString & operator += (const stdString &rhs);
	stdString & operator += (const char *rhs);
	stdString & operator += (char ch);
	friend stdString operator + (const stdString &lhs, const stdString &rhs);

	/// Comparisons
	///
	/// compare <  0: this <  rhs<br>
	/// compare >  0: this >  rhs<br>
	/// compare == 0: this == rhs
	int compare(const stdString &rhs) const;
	bool operator == (const stdString &rhs) const;    
	bool operator != (const stdString &rhs) const;
	bool operator < (const stdString &rhs) const;
	bool operator > (const stdString &rhs) const;
	bool operator <= (const stdString &rhs) const;
	bool operator >= (const stdString &rhs) const;

	/// Reserve space for string of given max. length.
	///
	/// Call in advance to make assignments and concatenations
	/// more effective
	bool reserve(size_type len);

	/// Get position [0 .. length()-1] of first/last ch
	///
	/// Retuns npos if not found
	size_type find(char ch) const; 
	size_type find(const stdString &s) const; 
	size_type find(const char *) const; 
	size_type find_last_of(char ch) const; 

	static const size_type npos; 

	/// Extract sub-string from position <I>from</I>, up to n elements
	stdString substr(size_type from = 0, size_type n = npos) const;

private:
	char	*_str;
	size_t	_len; // strlen() len (without endl)
	size_t	_res; // max len (allocated space = max len + 1)
};

// Implementation:
//
// _str initially 0:                          Yes
// _str can be "" (not deleted & reset to 0): Yes
// Reference counting:                        No
// 

inline const char *stdString::c_str () const
{	return _str ? _str : "";	}

inline stdString::stdString ()
{
	_str = 0;
	_len = 0;
	_res = 0;
}

inline stdString::stdString (const stdString &rhs)
{
	_str = 0;
	_len = 0;
	_res = 0;
	assign (rhs._str, rhs._len);
}

inline stdString::stdString (const char *s)
{
	_str = 0;
	_len = 0;
	_res = 0;
	if (s)
		assign (s, strlen (s));
}

inline stdString::~stdString ()
{
	if (_str)
		delete [] _str;
}

inline char stdString::operator [] (size_t index) const
{	return _str[index]; }

inline stdString & stdString::operator = (const stdString &rhs)
{
	if (&rhs == this)
		return *this;
	return assign (rhs._str, rhs._len);
}

inline stdString & stdString::operator = (const char *s)
{
	if (s)
		return assign (s, strlen (s));
	return assign (0, 0);
}

inline stdString & stdString::operator += (const stdString &rhs)
{
	if (rhs._len > 0)
	{
		if (reserve (_len + rhs._len))
		{
			memcpy (_str+_len, rhs._str, rhs._len);
			_len += rhs._len;
			_str[_len] = '\0';
		}
	}
	return *this;
}

inline stdString & stdString::operator += (const char *rhs)
{
    if (rhs)
    {
        size_t rhs_len = strlen(rhs);
        if (rhs_len > 0)
        {
            if (reserve (_len + rhs_len))
            {
                memcpy (_str+_len, rhs, rhs_len);
                _len += rhs_len;
                _str[_len] = '\0';
            }
        }
    }
    return *this;
}

inline stdString & stdString::operator += (char ch)
{
	if (reserve (_len + 1))
	{
		_str[_len] = ch;
		_len += 1;
		_str[_len] = '\0';
	}
	return *this;
}

inline stdString operator + (const stdString &lhs, const stdString &rhs)
{
    stdString   result;
	if (result.reserve (lhs._len + rhs._len))
	{
		result.assign (lhs._str, lhs._len);
		result += rhs;
	}
	return result;
}              

inline stdString::size_type stdString::length () const
{	return _len;	}

inline bool stdString::empty () const
{	return _len == 0; }

inline bool stdString::operator == (const stdString &rhs) const
{	return compare (rhs) == 0; }

inline bool stdString::operator != (const stdString &rhs) const
{	return compare (rhs) != 0; }

inline bool stdString::operator < (const stdString &rhs) const
{   return compare (rhs) < 0; }

inline bool stdString::operator > (const stdString &rhs) const
{   return compare (rhs) > 0; }

inline bool stdString::operator <= (const stdString &rhs) const
{   return compare (rhs) <= 0; }

inline bool stdString::operator >= (const stdString &rhs) const
{   return compare (rhs) >= 0; }

inline stdString::size_type stdString::find (char ch) const
{
	if (!_str)
		return npos;
	const char *pos = strchr (_str, ch);

	if (pos == 0)
		return npos;
	return pos - _str;
}

inline stdString::size_type stdString::find (const char *s) const
{
	if (!_str  ||  !s)
		return npos;

	const char *pos = strstr (_str, s);

	if (pos == 0)
		return npos;
	return pos - _str;
}

inline stdString::size_type stdString::find (const stdString &s) const
{	return find (s._str);	}

inline stdString::size_type stdString::find_last_of (char ch) const
{
	const char *pos = strrchr (_str, ch);

	if (pos == 0)
		return npos;
	return pos - _str;
}

#endif
