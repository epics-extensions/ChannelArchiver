proc ed {p} {
  switch $p {
    stop {return stopped}
    start {return started}
  }
}

proc httpd::sendCmdResponse {fd page ind} {
  puts $fd "<HTML><HEAD>
<meta http-equiv=refresh content=\"0; URL=http://[info hostname]:$::_port/\">
<TITLE>ArchiveEngine \"[camMisc::arcGet $ind descr]\" ${page}ed</TITLE>
</HEAD><BODY bgcolor=\"\#aec9d2\">
<TABLE BORDER=3><TR><TD BGCOLOR=#FFFFFF><FONT SIZE=5><em>[camMisc::arcGet $ind descr]</em> [ed ${page}]</FONT></TD></TR></TABLE>
<p>You'll be taken back to the main-page automatically in a few seconds...
</BODY></HTML>
"
  close $fd
}
