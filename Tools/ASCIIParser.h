// -------------------------------------------- -*- c++ -*-
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __ASCII_PARSER__
#define __ASCII_PARSER__

#include"ToolsConfig.h"
#include<stdio.h>

/// \ingroup Tools

/// Helper class for programming an ASCII-file parser:

/// The ASCIIParser assists in writing an ASCII file parser:
/// - Opens/closes file,
/// - skips comments (number sign),
/// - removes leading and trainling white space from lines,
/// - extracts parameter and value from "parameter=value" lines.
class ASCIIParser
{
public:
    ASCIIParser();
    
    virtual ~ASCIIParser();
    
    /// Open file for parsing.
    
    /// Result: true for success
    ///
    ///
    bool open(const stdString &file_name);

    /// Read next line from file, skipping comment lines.
    
    /// Result: false for error, hit end of file, ...
    ///
    ///
    bool nextLine();

    /// Get current line as string, excluding '\n'.

    ///
    ///
    ///
    const stdString & getLine() const;

    /// Get number of current line
    size_t getLineNo() const;

    /// Try to extract parameter=value pair from current line.
    
    /// Result: found parameter?
    ///
    ///
    bool getParameter(stdString &parameter, stdString &value);

private:
    size_t        _line_no;
    stdString     _line;
    FILE          *_file;
};

inline const stdString &ASCIIParser::getLine() const
{   return _line;   }

inline size_t ASCIIParser::getLineNo() const
{   return _line_no; }

#endif


