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

HTMLPage::HTMLPage()
{
    _started = false;
    _fill = true;
    _status = false;
    _round = 0;
    _interpol = 0;
}

void HTMLPage::start()
{
    std::cout << "Content-type: text/html\n"
              << "Pragma: no-cache\n"
              << "\n"
              << "<HTML>\n"
              << "<META Creator: EPICS ChannelArchiver CGIExport>\n"
              << "<HEAD>\n"
              << "<TITLE>" << _title << "</TITLE>\n";

    std::ifstream file;
    file.open(start_file);
#ifdef  __HP_aCC
    if (! file.fail())
#else
    if (file.is_open())
#endif
    {
        char line[200];
        while (! file.eof())
        {
            file.getline(line, sizeof (line));
            std::cout << line << "\n";
        }
    }
    else
    {
        std::cout << "\n<! default header --------->\n"
                  << "<BODY bgcolor=\"#FFFFFF\">\n"
                  << "<FONT face=\"Comic Sans MS, Arial, Helvetica\">\n"
                  << "<BLOCKQUOTE>\n"
                  << "<! create '" << start_file
                  << "' to replace this default header ---->\n\n";
    }
    _started = true;
}

static void makeSelect(const char *name, int start, int end, int select)
{
    std::cout << "           <SELECT NAME=" << name << " SIZE=1>\n";

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
    std::cout << "\n           </SELECT>\n";
}

// Print the interface stuff
void HTMLPage::interFace() const
{
    size_t i;
    int year, month, day, hour, min, sec;
    unsigned long nano;

    std::cout << "<FORM METHOD=\"GET\" ACTION=\"" << _cgi_path << "\">\n";
    std::cout << "  <INPUT TYPE=\"HIDDEN\" NAME=\"DIRECTORY\" VALUE=\""
              << _directory << "\">\n";
    std::cout << "  <TABLE cellpadding=1>\n";
    std::cout << "  <TR>\n";
    std::cout << "      <TD>Pattern:</TD>\n";
    std::cout << "      <TD><INPUT NAME=\"PATTERN\" VALUE=\""
              <<            _pattern << "\" SIZE=40 MAXLENGTH=100>\n";
    std::cout << "          <INPUT TYPE=submit NAME=COMMAND VALUE=LIST>\n";
    std::cout << "          <INPUT TYPE=submit NAME=COMMAND VALUE=INFO>\n";
    std::cout << "      </TD>\n";
    std::cout << "  </TR>\n";
    std::cout << "  <TR>\n";
    std::cout << "      <TD valign=\"top\">Names:</TD>\n";
    std::cout << "      <TD><TEXTAREA NAME=NAMES ROWS=5 COLS=40>";
    for (i=0; i<_names.size(); ++i)
        std::cout << _names[i] << "\n";
    std::cout << "</TEXTAREA>\n";
    std::cout << "      </TD>\n";
    std::cout << "  </TR>\n";

    osiTime2vals(_start, year, month, day, hour, min, sec, nano);
    std::cout << "  <TR>\n";
    std::cout << "      <TD>Start:</TD><TD>Day (m/d/y)\n";
    makeSelect("STARTMONTH",    1,   12, month);
    makeSelect("STARTDAY"  ,    1,   31, day);
    makeSelect("STARTYEAR" , 1998, 2005, year);
    std::cout << "Time (h:m:s)";
    makeSelect("STARTHOUR" ,    0,   23, hour);
    makeSelect("STARTMINUTE" ,  0,   59, min);
    makeSelect("STARTSECOND" ,  0,   59, sec);
    std::cout << "      </TD>\n";
    std::cout << "  </TR>\n";

    osiTime2vals(_end, year, month, day, hour, min, sec, nano);
    std::cout << "  <TR>\n";
    std::cout << "      <TD>End:</TD><TD>Day (m/d/y)\n";
    makeSelect("ENDMONTH",    1,   12, month);
    makeSelect("ENDDAY"  ,    1,   31, day);
    makeSelect("ENDYEAR" , 1998, 2005, year);
    std::cout << "Time (h:m:s)";
    makeSelect("ENDHOUR" ,    0,   23, hour);
    makeSelect("ENDMINUTE" ,  0,   59, min);
    makeSelect("ENDSECOND" ,  0,   59, sec);
    std::cout << "      </TD>\n";
    std::cout << "  </TR>\n";

    std::cout << "  <TR>\n";
    std::cout << "      <TD valign=\"top\">\n";
    std::cout << "          <input TYPE=submit NAME=COMMAND VALUE=GET>\n";
    std::cout << "      </TD>\n";
    std::cout << "      <TD>\n";
    std::cout << "        <TABLE cellpadding=2>\n";
    std::cout << "        <TR>\n";
    std::cout << "   	     <TD valign=top>Format:</td>\n";
    std::cout << "           <TD valign=top>"
              << "              <input type=radio name=FORMAT ";
    if (_format == "PLOT")
        std::cout << "checked=1 ";
    std::cout <<                "value=PLOT>Plot<br>\n";
    std::cout << "              <input type=radio name=FORMAT ";
    if (_format.empty() || _format == "SPREADSHEET")
        std::cout << "checked=1 ";
    std::cout <<                "value=SPREADSHEET"
              <<                ">Spreadsheet<br>\n";
    std::cout << "              <input type=radio name=FORMAT ";
    if (_format == "EXCEL")
        std::cout << "checked=1 ";
    std::cout <<                "value=EXCEL>Excel-File<br>\n";
    std::cout << "              <input type=radio name=FORMAT ";
    if (_format == "MATLAB")
        std::cout << "checked=1 ";
    std::cout      <<                "value=MATLAB>Matlab\n";
    std::cout << "           </td>\n";
    std::cout << "           <td></td>\n";
    std::cout << "           <td valign=top align=right>\n";
    std::cout <<                 "Status:<br>\n";
    std::cout <<                 "Fill:<br>\n";
    std::cout <<                 "Interpolate:<br>\n";
    std::cout << "           </td>\n";
    std::cout << "           <td>\n";
    std::cout << "              <input name=STATUS type=checkbox value=ON> (show channel status)<br>\n";
    std::cout << "              <input name=FILL type=checkbox value=ON> (step-func. interpolation)<br>\n";
    std::cout << "              <input maxLength=10 name=INTERPOL size=5 value=0> secs (linear)\n";
    std::cout << "           </TD>\n";
    std::cout << "        </TR>\n";
    std::cout << "        </TABLE>\n";
    std::cout << "      </TD>\n";
    std::cout << "  </TR>\n";
    std::cout << "  </TABLE>\n";
    std::cout << "</FORM>\n";
}

HTMLPage::~HTMLPage()
{
    if (_started)
    {
        std::ifstream file;
        file.open(end_file);
#ifdef  __HP_aCC
        if (! file.fail())
#else
        if (file.is_open())
#endif
        {
            char line[200];
            while (! file.eof())
            {
                file.getline(line, sizeof (line));
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

void HTMLPage::header(const stdString &text, int level) const
{
    std::cout << "<H" << level << ">" << text << "</H" << level << ">\n";
}




