$ATAC="../../O.solaris/atac";
$ATAC="bltwish";

$ARGC=$#ARGV + 1;

die "Usage: $0 [yyyy mm dd]\n" unless (($ARGC == 0) || ($ARGC == 3));

if ($ARGC==0)
{
	# Default: yesterday
	($sec, $min, $hour, $day, $month, $year) = localtime(time - 60*60*24);
	$month += 1;
	if ($year < 90)
	{	$year += 2000; }
	else
	{	$year += 1900; }
}
else
{
	$year = $ARGV[0];
	$month = $ARGV[1];
	$day = $ARGV[2];
}

$monthdirname=sprintf ("%04d%02d", $year, $month);
$daydirname=sprintf ("%04d%02d/%02d", $year, $month, $day);
$date=sprintf ("%04d/%02d/%02d", $year, $month, $day);
$start="$date 00:00:00";
$end="$date 23:59:59";

# print "Creating plot in directory $dirname\n";

system "mkdir $monthdirname";
system "mkdir $daydirname";
system "cd $daydirname;$ATAC ../../beam.tcl     \"$start\" \"$end\"";

$PAGE=">$daydirname/default.htm";

open PAGE or die "Cannot create $PAGE\n";

print PAGE "<HTML>";
print PAGE "<META Created by $0 to display beam plots>";
print PAGE "<HEAD>";
print PAGE "<TITLE>Beam Statistics</TITLE>";
print PAGE "</HEAD>";
print PAGE "";
print PAGE "<BODY  BGCOLOR=#F0F0FF>";
print PAGE "<FONT FACE=\"Comic Sans MS,Arial, Helvetica\">";
print PAGE "<BLOCKQUOTE>";
print PAGE "<H1><IMG SRC=\"../../apt_sm.gif\" height=51 width=82 align=center><FONT Color=\"#1000A0\">LEDA Beam Statistics</FONT></H1>\n";
print PAGE "<H2>Beam Current</H2>";
print PAGE "<IMG SRC=current.gif>";
print PAGE "<H2>Beam \"On\" Time</H2>";
print PAGE "<IMG SRC=periods.gif>";
print PAGE "<HR>";
print PAGE "<ADDRESS>";
print PAGE "<A HREF=\"mailto:kasemir\@lanl.gov\">kasemir\@lanl.gov</A>";
print PAGE "</ADDRESS>";
print PAGE "</BLOCKQUOTE>";
print PAGE "</FONT>";
print PAGE "</BODY>";
print PAGE "</HTML>";


