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

USE_STD_NAMESPACE
USING_NAMESPACE_TOOLS

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
    cout << "Content-type: text/html\n";
    cout << "Pragma: no-cache\n";
    cout << "\n";
    cout << "<HTML>\n";
    cout << "<HEAD>\n";
    cout << "<TITLE>" << _title << "</TITLE>\n";

    ifstream file;
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
            cout << line << endl;
        }
    }
    else
    {
        cout << "\n<! default header --------->\n";
        cout << "<BODY bgcolor=\"#FFFFFF\">\n";
        cout << "<FONT face=\"Comic Sans MS, Arial, Helvetica\">\n";
        cout << "<BLOCKQUOTE>\n";
        cout << "<! create '" << start_file << "' to replace this default header ---->\n\n";
    }
    _started = true;
}

static void makeSelect (const char *name, int start, int end, int select)
{
    cout << "<! select: " << select << ">\n";
    cout << "<SELECT NAME=" << name << " SIZE=1>\n";

    for (int i=start; i<=end; ++i)
    {
        cout << "<OPTION";
        if (i==select)
            cout << " SELECTED";
        cout << "> ";
        if (i<10)
            cout << "0";
        cout << i << "</OPTION> ";
    }
    cout << "</SELECT>\n";
}

// Print the interface stuff
void HTMLPage::interFace () const
{
    size_t i;
    int year, month, day, hour, min, sec;
    unsigned long nano;

    cout << "<FORM METHOD=\"GET\" ACTION=\"" << _cgi_path << "\">\n";
    cout << "<INPUT TYPE=\"HIDDEN\" NAME=\"DIRECTORY\" VALUE=\"" << _directory << "\">\n";
    cout << "<TABLE cellpadding=1>\n";
    cout << "<TR><TD>Pattern:</TD><TD><INPUT TYPE=\"TEXT\" NAME=\"PATTERN\" VALUE=\"" <<
            _pattern << "\" SIZE=40 MAXLENGTH=100> (regular expression)</TD></TR>\n";
    cout << "<TR><TD>Names:</TD><TD><TEXTAREA NAME=NAMES ROWS=5 COLS=40>";
    for (i=0; i<_names.size(); ++i)
        cout << _names[i] << "\n";
    cout << "</TEXTAREA></TD></TR>\n";
    cout << "<TR><TD>Interpolate:</TD><TD><INPUT TYPE=TEXT NAME=INTERPOL VALUE=\"" << _interpol << "\" SIZE=5 MAXLENGTH=10> secs,   ";
    cout << "Round: <INPUT TYPE=TEXT NAME=ROUND VALUE=\"" << _round << "\" SIZE=5 MAXLENGTH=10> secs,   ";
    cout << "Fill:<INPUT TYPE=CHECKBOX NAME=FILL";
    if (_fill)
        cout << " CHECKED";
    cout << ">  Status:<INPUT TYPE=CHECKBOX NAME=STATUS";
    if (_status)
        cout << " CHECKED";
    cout << "></TD></TR>\n";

    osiTime2vals (_start, year, month, day, hour, min, sec, nano);
    cout << "<TR><TD>Start:</TD><TD>Day (m/d/y)\n";
    makeSelect ("STARTMONTH",    1,   12, month);
    makeSelect ("STARTDAY"  ,    1,   31, day);
    makeSelect ("STARTYEAR" , 1998, 2003, year);
    cout << "Time (h:m:s)";
    makeSelect ("STARTHOUR" ,    0,   23, hour);
    makeSelect ("STARTMINUTE" ,  0,   59, min);
    makeSelect ("STARTSECOND" ,  0,   59, sec);
    cout << "</TD></TR>\n";

    osiTime2vals (_end, year, month, day, hour, min, sec, nano);
    cout << "<TR><TD>End:</TD><TD>Day (m/d/y)\n";
    makeSelect ("ENDMONTH",    1,   12, month);
    makeSelect ("ENDDAY"  ,    1,   31, day);
    makeSelect ("ENDYEAR" , 1998, 2003, year);
    cout << "Time (h:m:s)";
    makeSelect ("ENDHOUR" ,    0,   23, hour);
    makeSelect ("ENDMINUTE" ,  0,   59, min);
    makeSelect ("ENDSECOND" ,  0,   59, sec);
    cout << "</TD></TR>\n";

    cout << "<TR><TD>Command:</TD>\n";
    cout << "<TD><TABLE CELLPADDING=3>\n";
    cout << "   <TR>\n";
    cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=LIST> </TD>\n";
    cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=INFO> </TD>\n";
    cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=PLOT> </TD>\n";
    cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=GET>  </TD>\n";
    cout << "   <TD><input TYPE=submit NAME=COMMAND VALUE=DEBUG></TD>\n";
    cout << "   </TR>\n";
    cout << "   </TABLE>\n";
    cout << "</TD>\n";
    cout << "</TR>\n";

    cout << "</TABLE>\n";
    cout << "</FORM>\n";
}

HTMLPage::~HTMLPage ()
{
    if (_started)
    {
        ifstream file;
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
                cout << line << endl;
            }
        }
        else
        {
            cout << "\n<! default footer --------->\n";
            cout << "</BLOCKQUOTE>\n";
            cout << "</FONT>\n";
            cout << "</BODY>\n";
            cout << "<! create '" << end_file << "' to replace this default footer ---->\n\n";
        }
        cout << "</HTML>\n";
    }
}

void HTMLPage::header (const stdString &text, int level) const
{
    cout << "<H" << level << ">" << text << "</H" << level << ">\n";

}

