proc httpd::sendError {fd msg} {
  variable _proto
  variable _query
  puts $fd "<HTML><HEAD>
<TITLE>$msg</TITLE>
</HEAD><BODY bgcolor=\"\#aec9d2\">
<TABLE BORDER=3><TR><TD BGCOLOR=#FFFFFF><FONT SIZE=5>$msg</FONT></TD></TR></TABLE>
<PRE>
$_query($fd)
</PRE>
</BODY></HTML>"
  close $fd
}
