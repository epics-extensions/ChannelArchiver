// GenericException.h: interface for the GenericException class.
//////////////////////////////////////////////////////////////////////

#if !defined(_GENERICEXCEPTION_H_)
#define _GENERICEXCEPTION_H_

#include <ToolsConfig.h>

//////////////////////////////////////////////////////////////////
// Generic Exception: Base class, signals a general problem.
//
// Exception that provided info text with sourcefile & line information
//
// All exceptions should be caught by reference
// to avoid unnecessary copies and assert de-allocation:
//
//  try
//  {
//      ... throw GenericException(__FILE__, __LINE__);
//  }
//  catch (GenericException &e)
//  {
//      fprintf(stderr, "Exception:\n%s\n", e.what);
//  }
//
// (According to Scott Meyers "More Effective C++
//  a copy will be thrown, but Visual C++ seems to
//  efficiently throw without copying).
//
// It's a good idea to print the exception information as given above
// with the e.what() - string on a new line.
// That line will then usually read "filename (line-#)"
// and this format allows easy lookup from within IDEs.
//////////////////////////////////////////////////////////////////
class GenericException
{
public:
    GenericException(const char *sourcefile, size_t line)
        : _sourcefile(sourcefile), _line(line)
    {}

    virtual ~GenericException()
    {}

    // Retrieve an explanatory string.
    // Default implementation will give source file and line
    // in a format that's parsed by e.g. Visual Studio:
    // "%s (%d) ....", SourceFile (), SourceLine ()
    virtual const char *what() const;

    // Source file where exception was thrown
    const char *getSourceFile() const
    {   return _sourcefile; }

    // Line in SourceFile() where exception was thrown
    size_t getSourceLine() const
    {   return _line;   }

protected:
    // Buffer for ErrorInfo.
    // Text handles deletion of string memory
    // when GenericException is deleted.
    // Might be generated on demand in what() -> mutable.
    mutable stdString _error_info;

private:
    const char  *_sourcefile;
    size_t      _line;

};

#endif // !defined(_GENERICEXCEPTION_H_)

