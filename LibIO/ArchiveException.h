// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __ARCHIVEEXCEPTION_H__
#define __ARCHIVEEXCEPTION_H__

#include "ArchiveTypes.h"
#include<stdio.h>

//CLASS ArchiveException
// Special Exception for ChanArch classes.
//
// All Exceptions should be caught by reference
// to avoid unnecessary copies and assert deallocation:
// <PRE>
//	try
//	{
//		... throwArchiveException (Invalid);
//	}
//	catch (const ArchiveException &e)
//	{
//		LOG_MSG ("%s\n", e.what ());
//	}
// </PRE>
//
// ArchiveException is based on Tools::GenericException
// which - on systems where it's properly defined - is
// in turn based on std::exception.
class ArchiveException : public GenericException
{
public:
	typedef enum
	{
		NoError,	// This should never occur...
		Fail,		// Failure
		Invalid,	// Invalid: not initialized, wrong type, ...
		OpenError,	// Cannot open existing new file
		CreateError,// Cannot create new file
		ReadError,	// Read Error
		WriteError,	// Write Error
		Unsupported // Not Supported (dbr type,...)
	} Code;

	//* Instead of using this constructor the predefined
	// macro <I>throwArchiveException(code)</I> should be used.
	ArchiveException (const char *sourcefile, size_t line, Code code)
	: GenericException(sourcefile, line), _code (code)
	{}

	//* Similar, use 
	// <I>throwDetailedArchiveException(code, info)</I>.
	ArchiveException(const char *sourcefile, size_t line,
                     Code code, const stdString &detail)
	: GenericException(sourcefile, line), _code (code), _detail (detail)
	{}

	//* For valid codes see <I>ArchiveException.h</I>.
	Code getErrorCode () const;

	//* Return detail text (if set)
	const stdString &getDetail () const;

	//* Returns explanatory text for this exception.
	virtual const char *what() const;

private:
	Code _code;
	stdString _detail;
};

inline ArchiveException::Code ArchiveException::getErrorCode() const
{	return _code;	}

inline const stdString &ArchiveException::getDetail() const
{	return _detail; }



// inline? Then e.g. Invalid would have to be given as
// ChannelArchiveException::Invalid...
#define throwArchiveException(code)	\
   throw ArchiveException(__FILE__, __LINE__,ArchiveException::code)
#define throwDetailedArchiveException(code,detail) \
   throw ArchiveException(__FILE__, __LINE__,ArchiveException::code, detail)

#endif //__ARCHIVEEXCEPTION_H__
