%title "CASI - ChannelArchiver Scripting Interface", before
%style html_body="<BODY bgcolor=\"#B0B0FF\"><BLOCKQUOTE><FONT face=\"Comic Sans MS\">:</FONT></BLOCKQUOTE></BODY>"


%module casi

%text %{
This scriping interface is meant to mimic the
behaviour of the C++ LibIO API to the ChannelArchiver
as close as possible.
Therefore you might consider crosschecking with
that documentation.
%}

/* Includes for compilation of wrapper */
%{
#include <MultiArchive.h>
USE_STD_NAMESPACE
USING_NAMESPACE_CHANARCH
#include "archive.h"
#include "channel.h"
#include "value.h"
%}

/* Following includes are used by SWIG for generation of wrapper */

const char *casi_version = "1.0";

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

%section "Questions, comments?"
%text %{
Let me know: Kay-Uwe Kasemir, kasemir@lanl.gov
%}







