/*  -*- mode: C++ -*-          <-              for emacs */

%title "CASI - ChannelArchiver Scripting Interface"
%style html_body="<BODY bgcolor=\"#B0B0FF\"><BLOCKQUOTE><FONT face=\"Comic Sans MS\">:</FONT></BLOCKQUOTE></BODY>"

%module casi

%text %{
This scriping interface is meant to mimic the
behaviour of the C++ LibIO API to the ChannelArchiver
as close as possible.
Therefore you might consider crosschecking with
that documentation.


Most functions can generate a RuntimeError or
an UnknownError.


%}

/* Includes for compilation of wrapper */
%{
/* Perl: The perl header files use list in a very unfortunate
 *       manner. Existing definition of assert leads to warnings.
 */
#undef list
#undef assert
#undef open
#undef write
#undef read
#undef eof

#undef setbuf


#include "ToolsConfig.h"
#include "GenericException.h"
class ArchiveI;
class ChannelIteratorI;
class ValueIteratorI;

#include "archive.h"
#include "channel.h"
#include "value.h"
%}

/* Following includes are used by SWIG for generation of wrapper */

const char *casi_version = "1.0";

%include exception.i

%except
{
    try
    {
        $function
    }
    catch (GenericException &e)
    {   // un-const to avoid warnings
        SWIG_exception (SWIG_RuntimeError, (char *) e.what());
    }
    catch (...)
    {
        SWIG_exception (SWIG_UnknownError, "Unknown exception while calling CASI");
    }
}

/* The archive class is the starting point:

   Open an archive first, then look for specific channels
   by name or loop over all the channels
   (maybe restricted by regular expression).

 */
%section "Accessing an Archive"

%include "archive.h"

/* The channel class provides information about a single channel.

   It allows searching for values relative to some point in time
 */
%section "Channel Information"

%include "channel.h"

/* The value class provides access to a single value,
   consisting of a time stamp, a value and status information.
 */
%section "Value Information"

%include "value.h"

/* Helpers specific to the BinArchive
 */
%section "Misc."

const int HOURS_PER_MONTH = 24*31; /* depends on month, ... this is
                                      a magic number */

%section "Questions, comments?"
%text %{
Let me know: Kay-Uwe Kasemir, kasemir@lanl.gov
%}







