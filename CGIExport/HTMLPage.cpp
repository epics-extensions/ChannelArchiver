// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "HTMLPage.h"
#include <iostream>
#include <fstream>
#include "osiTimeHelper.h"

static const char *start_file = "cgi_body_start.txt";
static const char *end_file = "cgi_body_end.txt";

HTMLPage::HTMLPage ()
{
    _started = false;
    _fill = true;
    _status = false;
    _round = 0;
    _interpol = 0;
}

void HTMLPage::start ()
{
    std::cout << "Content-type: text/html\n"
              << "Pragma: no-cache\n"
              << "\n"
              << "<HTML>\n"
              << "<HEAD>\n"
              << "<TITLE>" << _title << "</TITLE>\n";

    std::ifstream file;
    file.open (start_file);
#ifdef  __HP_aCC
    if (! file.fail())
#else
    if (file.is_open ())
#endif
    {
        char line[200];
        while (! file.eof())
        {
            file.getline (line, sizeof (line));
            std::cout << line << "\n";
        }
    }
    else
    {
        std::cout << "\n<! default header --------->\n"
                  << "<BODY bgcolor=\"#FFFFFF\">\n"
                  << "<FONT face=\"Comic Sans MS, Arial, Helvetica\">\n"
                  << "<BLOCKQUOTE>\n"
                  << "<! create '" << start_file << "' to replace this default header ---->\n\n";
    }
    _started = true;
}

static void makeSelect (const char *name, int start, int end, int select)
{
    std::cout << "<! select: " << select << ">\n"
              << "<SELECT NAME=" << name << " SIZE=1>\n";

    for (int i=start; i<=end; ++i)
    {
        std::cout << "<OPTION";
        if (i==select)
            std::cout << " SELECTED";
        std::cout << "> ";
        if (i<10)
            std::cout << "0";
        std::cout << i << "</OPTION> ";
    }
    std::cout << "</SELECT>\n";
}

// Print the interface stuff
void HTMLPage::interFace () const
{
    size_t i;
    int year, month, day, hour, min, sec;
    unsigned long nano;

    std::cout << "<FORM METHOD=\"GET\" ACTION=\"" << _cgi_path << "\">\n";
    std::cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"DIRECTORY\" VALUE=\"" << _directory << "\">\n";
    std::cout << "<TABLE cellpadding=1>\n";
    std::cout << "<TR><TD>Pattern:</TD><TD><INPUT TYPE=\"TEXT\" NAME=\"PATTERN\" VALUE=\"" <<
            _pattern << "\" SIZE=40 MAXLENGTH=100> (regular expression)</TD></TR>\n";
    std::cout << "<TR><TD>Names:</TD><TD><TEXTAREA NAME=NAMES ROWS=5 COLS=40>";
    for (i=0; i<_names.size(); ++i)
        std::cout << _names[i] << "\n";
    std::cout << "</TEXTAREA></TD></TR>\n";
    std::cout << "<TR><TD>Interpolate:</TD><TD><INPUT TYPE=TEXT NAME=INTERPOL VALUE=\"" << _interpol << "\" SIZE=5 MAXLENGTH=10> secs,   ";
    std::cout << "Round: <INPUT TYPE=TEXT NAME=ROUND VALUE=\"" << _round << "\" SIZE=5 MAXLENGTH=10> secs,   ";
    std::cout << "Fill:<INPUT TYPE=CHECKBOX NAME=FILL";
    if (_fill)
        std::cout << " CHECKED";
    std::cout << ">  Status:<INPUT TYPE=CHECKBOX NAME=STATUS";
    if (_status)
        std::cout << " CHECKED";
    std::cout << "></TD></TR>\n";

    osiTime2vals (_start, year, month, day, hour, min, sec, nano);
    std::cout << "<TR><TD>Start:</TD><TD>Day (m/d/y)\n";
    makeSelect ("STARTMONTH",    1,   12, month);
    makeSelect ("STARTDAY"  ,    1,   31, day);
    makeSelect ("STARTYEAR" , 1998, 2003, year);
    std::cout << "Time (h:m:s)";
    makeSelect ("STARTHOUR" ,    0,   23, hour);
    makeSelect ("STARTMINUTE" ,  0,   59, min);
    makeSelect ("STARTSECOND" ,  0,   59, sec);
    std::cout << "</TD></TR>\n";

    osiTime2vals (_end, year, month, day, hour, min, sec, nano);
    std::cout << "<TR><TD>End:</TD><TD>Day (m/d/y)\n";
    makeSelect ("ENDMONTH",    1,   12, month);
    makeSelect ("ENDDAY"  ,    1,   31, day);
    makeSelect ("ENDYEAR" , 1998, 2003, year);
    std::cout << "Time (h:m:s)";
    makeSelect ("ENDHOUR" ,    0,   23, hour);
    makeSelect ("ENDMINUTE" ,  0,   59, min);
    makeSelect ("ENDSECOND" ,  0,   59, sec);
    std::cout << "</TD></TR>\n";

    std::cout << "<TR><TD>Command:</TD>\n";
    std::cout << "<TD><TABLE CELLPADDING=3>\n";
    std::cout << "   <TR>\n";
    std::cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=LIST> </TD>\n";
    std::cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=INFO> </TD>\n";
    std::cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=PLOT> </TD>\n";
    std::cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=GET>  </TD>\n";
    std::cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=DEBUG></TD>\n";
    std::cout << "   </TR>\n";
    std::cout << "   </TABLE>\n";
    std::cout << "</TD>\n";
    std::cout << "</TR>\n";

    std::cout << "</TABLE>\n";
    std::cout << "</FORM>\n";
}

HTMLPage::~HTMLPage ()
{
    if (_started)
    {
        std::ifstream file;
        file.open (end_file);
#ifdef  __HP_aCC
        if (! file.fail())
#else
        if (file.is_open ())
#endif
        {
            char line[200];
            while (! file.eof())
            {
                file.getline (line, sizeof (line));
                std::cout << line << "\n";
            }
        }
        else
        {
            std::cout << "\n<! default footer --------->\n";
            std::cout << "</BLOCKQUOTE>\n";
            std::cout << "</FONT>\n";
            std::cout << "</BODY>\n";
            std::cout << "<! create '" << end_file << "' to replace this default footer ---->\n\n";
        }
        std::cout << "</HTML>\n";
    }
}

void HTMLPage::header (const stdString &text, int level) const
{
    std::cout << "<H" << level << ">" << text << "</H" << level << ">\n";
}

