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
class ValueI;

#include "archive.h"
#include "channel.h"
#include "value.h"
#include "ctrlinfo.h"
%}

/* Following includes are used by SWIG for generation of wrapper */

%section "ChannelArchiver related Variables"

const char *casi_version = "1.0";

%subsection "Datatypes"
#define DBR_TIME_STRING 14
#define DBR_TIME_INT    15
#define DBR_TIME_SHORT  15
#define DBR_TIME_FLOAT  16
#define DBR_TIME_ENUM   17
#define DBR_TIME_CHAR   18
#define DBR_TIME_LONG   19
#define DBR_TIME_DOUBLE 20

%subsection "special values for severity"
#define ARCH_NO_VALUE 		0x0f00
#define ARCH_EST_REPEAT 	0x0f80
#define ARCH_DISCONNECT 	0x0f40
#define ARCH_STOPPED 		0x0f20
#define ARCH_REPEAT 		0x0f10
#define ARCH_DISABLED 		0x0f08
#define ARCH_CHANGE_WRITE_FREQ 	0x0f04
#define ARCH_CHANGE_PERIOD 	0x0f02
#define ARCH_CHANGE_SIZE 	0x0f01


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

%section "Accessing an Archive"
/* The archive class is the starting point:

   Open an archive first, then look for specific channels
   by name or loop over all the channels
   (maybe restricted by regular expression).

 */

%include "archive.h"

%section "Channel Information"
/* The channel class provides information about a single channel.

   It allows searching for values relative to some point in time
 */

%include "channel.h"

%section "Value Information"
/* The value class provides access to a single value,
   consisting of a time stamp, a value and status information.
 */

%include "value.h"

%section "Control Information"
/* The ctrlinfo class provides access to the conrtol-info of a value.
 */

%include "ctrlinfo.h"


%section "Misc."
/* Helpers specific to the BinArchive
 */

const int HOURS_PER_MONTH = 24*31; /* depends on month, ... this is
                                      a magic number */

%section "Questions, comments?"
%text %{
Let me know: Kay-Uwe Kasemir, kasemir@lanl.gov
%}







