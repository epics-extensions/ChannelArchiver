// -------------- -*- c++ -*-
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __ARGPARSER_H__
#define __ARGPARSER_H__

#include <ToolsConfig.h>

class CmdArg;

// CLASS CmdArgParser
class CmdArgParser
{
public:
    //* Initialize CmdArgParser for argc/argv
    CmdArgParser(int argc, char const * const *argv);

    void addOption(CmdArg *option);

    void usage();

    //* Set additional header/footer text
    // (default: nothing)
    void setHeader(const char *header);
    void setFooter(const char *footer);

    //* Description for remaining arguments (other than options)
    void setArgumentsInfo(const char *args_info);

    //* Parse Arguments,
    // result: false on error
    // (usage will be shown)
    bool parse();

    //* Remaining arguments (w/o switches)
    const stdVector<const char *> &getArguments();
    const char *getArgument(size_t i);

private:
    const char *_header;
    const char *_footer;
    const char *_args_info;
    const char *_progname;        // Name of Program
    int _argc;                    // Original argc/argv minus _progname
    char const * const *_argv;
    stdList<CmdArg *> _options;      // Available options
    stdVector<const char *> _args;   // remaining args
};

// CLASS CmdArg
class CmdArg
{
public:
    CmdArg(CmdArgParser &args,
           const char *option,
           const char *arguments,
           const char *description);
    virtual ~CmdArg();

    //* Called with option (minus '-'),
    // determine how many characters match
    size_t findMatches(const char *option) const;

    //* Show info on option for command line
    void usage_option() const;

    //* Show option description
    void usage() const;

    //* Parse arguments from current option.
    // args: following arg
    // Result: 0: error
    //         1: option ok, no argument needed
    //         2: option ok, argument swallowed
    virtual size_t parse(const char *arg) = 0;

protected:
    const char *_option;
    const char *_arguments;
    const char *_description;
};

// CLASS CmdArgFlag
class CmdArgFlag : public CmdArg
{
public:
    CmdArgFlag(CmdArgParser &args,
               const char *option, const char *description);
    
    operator bool() const;
    void set(bool value=true)
    {   _value = value; }

    virtual size_t parse(const char *arg);
private:
    bool _value;
};

// CLASS CmdArgInt
class CmdArgInt : public CmdArg
{
public:
    CmdArgInt(CmdArgParser &args, const char *option,
               const char *argument_name, const char *description);
    
    void set(int value);
    operator int() const;
    int get() const;

    virtual size_t parse(const char *arg);

private:
    int _value;
};

// CLASS CmdArgDouble
class CmdArgDouble : public CmdArg
{
public:
    CmdArgDouble(CmdArgParser &args, const char *option,
                 const char *argument_name, const char *description);
    
    void set(double value);
    operator double() const;

    virtual size_t parse(const char *arg);
private:
    double _value;
};

// CLASS CmdArgString
class CmdArgString : public CmdArg
{
public:
    CmdArgString(CmdArgParser &args, const char *option,
                 const char *argument_name, const char *description);
    
    void set(const stdString &value);
    operator const stdString &() const;
    const stdString &get() const;

    virtual size_t parse(const char *arg);
    
private:
    stdString _value;
};

// inlines ----------------------------------------------

inline void CmdArgParser::setHeader(const char *header)
{   _header = header; }

inline void CmdArgParser::setFooter(const char *footer)
{   _footer = footer; }

inline void CmdArgParser::setArgumentsInfo(const char *args_info)
{   _args_info = args_info; }

inline const stdVector<const char *> &CmdArgParser::getArguments()
{   return _args; }

inline const char *CmdArgParser::getArgument(size_t i)
{   return _args[i]; }


inline CmdArgFlag::operator bool() const
{   return _value; }

inline void CmdArgInt::set(int value)
{   _value = value; }

inline CmdArgInt::operator int() const
{   return _value; }
 
inline int CmdArgInt::get() const
{   return _value; }

inline void CmdArgDouble::set(double value)
{   _value = value; }

inline CmdArgDouble::operator double() const
{   return _value; }

inline void CmdArgString::set(const stdString &value)
{   _value = value; }

inline CmdArgString::operator const stdString &() const
{   return _value; }
 
inline const stdString &CmdArgString::get() const
{   return _value; }

#endif //  __ARGPARSER_H__

