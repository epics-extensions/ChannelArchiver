// GenericException.cpp: implementation of the GenericException class.
//////////////////////////////////////////////////////////////////////

#include"GenericException.h"
#include<stdio.h>

//////////////////////////////////////////////////////////////////////
// GenericException
//////////////////////////////////////////////////////////////////////
const char *GenericException::what() const
{
	if (_error_info.empty())
	{
        char buffer[1024];
        sprintf(buffer, "%s (%d): GenericException",
                _sourcefile, _line);
		_error_info = buffer;
	}
	return _error_info.c_str();
}

//////////////////////////////////////////////////////////////////////
// According to the C++ standard,
// new should throw bad_alloc on failure...
//////////////////////////////////////////////////////////////////////

// From MS Knowledge Base:
//
// PRB: Operator New Doesn't Throw bad_alloc Exception on Failure
// Last reviewed: July 24, 1997
// Article ID: Q167733 
//
// Operator new does not throw a bad_alloc exception
// when it fails. It simply returns a null pointer. 
// This behavior is by design
// though not in conformance with the ANSI Draft Working Papers
// for C++. 
#if _MSC_VER > 1000

#include <new>
#include <new.h>

// assert this gets called before any other global object
#pragma warning (disable: 4073)
#pragma init_seg(lib)
#pragma warning (default: 4073)

static int my_new_handler(size_t size)
{
    throw std::bad_alloc();
	return 0;
}

class my_new_handler_obj
{
public:
	my_new_handler_obj()
	{
		_old_new_mode = _set_new_mode(1); // cause malloc to throw like new
		_old_new_handler = _set_new_handler(my_new_handler);
	}

	~my_new_handler_obj()
	{
		_set_new_handler(_old_new_handler);
		_set_new_mode(_old_new_mode);
	}

private:
	_PNH _old_new_handler;
	int _old_new_mode;
};

static my_new_handler_obj _g_new_handler_obj;

#endif


