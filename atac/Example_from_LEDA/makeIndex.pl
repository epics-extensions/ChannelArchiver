################################
# Master index of all months
################################
$INDEX=">index.htm";

open INDEX or die "Cannot open index file";

printf INDEX "<HTML>\n";
printf INDEX "<HEAD>\n";
printf INDEX "<TITLE>Beam Statistics</TITLE>\n";
printf INDEX "</HEAD>\n";
printf INDEX "\n";
printf INDEX "<BODY  BGCOLOR=#F0F0FF>\n";
printf INDEX "<FONT FACE=\"Comic Sans MS,Arial, Helvetica\">\n";
printf INDEX "<BLOCKQUOTE>\n";
printf INDEX "<H1><IMG SRC=\"apt_sm.gif\" height=51 width=82 align=center><FONT Color=\"#1000A0\">\n";
printf INDEX "LEDA Beam Statistics</FONT></H1>\n";
printf INDEX "<H2>Available Plots:</H2>\n";
printf INDEX "<UL>\n";

foreach $defaultfile ( glob "*/*/default.htm" )
{
	if ($defaultfile =~ m"([0-9][0-9][0-9][0-9])([0-9][0-9])/([0-9][0-9])")
	{
		$year_month = "$1$2";
		$date_text{$year_month} = "$2/$1";
		$day = $3;
		$file{$year_month}{$day} = "$day/default.htm";

		printf "$year_month, $day: $file{$year_month}{$day}\n";
	}
}

foreach $year_month ( sort keys %file)
{
	$date = $year_month;
	printf INDEX "<LI><A HREF=\"$year_month/index.htm\">$date_text{$year_month}</A>\n";
}

printf INDEX "</UL>\n";
printf INDEX "<HR>\n";
printf INDEX "<ADDRESS>\n";
printf INDEX "The pages referenced by this index\n";
printf INDEX "were created with TCL scripts,<BR>\n";
printf INDEX "based on examples given by Larry Rybarcyk ";
printf INDEX "(<A HREF=\"mailto:lrybarcyk\@lanl.gov\">lrybarcyk\@lanl.gov</A>).<BR>\n";

printf INDEX "For more information on how to fix problems or create\n";
printf INDEX "customized scripts, ask \n";

printf INDEX "<A HREF=\"mailto:kasemir\@lanl.gov\">kasemir\@lanl.gov</A>\n";
printf INDEX "</ADDRESS>\n";
printf INDEX "</BLOCKQUOTE>\n";
printf INDEX "</FONT>\n";
printf INDEX "</BODY>\n";
printf INDEX "</HTML>\n";

close INDEX;

################################
# Indices for each month
################################

foreach $year_month ( sort keys %file)
{
	$INDEX=">$year_month/index.htm";

	open INDEX or die "Cannot open index file $INDEX";

	printf INDEX "<HTML>\n";
	printf INDEX "<HEAD>\n";
	printf INDEX "<TITLE>Beam Statistics $date_text{$year_month}</TITLE>\n";
	printf INDEX "</HEAD>\n";
	printf INDEX "\n";
	printf INDEX "<BODY  BGCOLOR=#F0F0FF>\n";
	printf INDEX "<FONT FACE=\"Comic Sans MS,Arial, Helvetica\">\n";
	printf INDEX "<BLOCKQUOTE>\n";
	printf INDEX "<H1><IMG SRC=\"../apt_sm.gif\" height=51 width=82 align=center><FONT Color=\"#1000A0\">\n";
	printf INDEX "LEDA Beam Statistics for $date_text{$year_month}</FONT></H1>\n";
	printf INDEX "<H2>Available Days for $date_text{$year_month}:</H2>\n";
	printf INDEX "<UL>\n";

	foreach $day ( sort keys %{ $file{$year_month} } )
	{
		$date = "$2/$3/$1";
		printf INDEX "<LI><A HREF=\"$file{$year_month}{$day}\">$day</A>\n";
	}

	printf INDEX "</UL>\n";
	printf INDEX "<HR>\n";
	printf INDEX "<ADDRESS>\n";
	printf INDEX "The pages referenced by this index\n";
	printf INDEX "were created with TCL scripts,<BR>\n";
	printf INDEX "based on examples given by Larry Rybarcyk ";
	printf INDEX "(<A HREF=\"mailto:lrybarcyk\@lanl.gov\">lrybarcyk\@lanl.gov</A>).<BR>\n";

	printf INDEX "For more information on how to fix problems or create\n";
	printf INDEX "customized scripts, ask \n";

	printf INDEX "<A HREF=\"mailto:kasemir\@lanl.gov\">kasemir\@lanl.gov</A>\n";
	printf INDEX "</ADDRESS>\n";
	printf INDEX "</BLOCKQUOTE>\n";
	printf INDEX "</FONT>\n";
	printf INDEX "</BODY>\n";
	printf INDEX "</HTML>\n";

	close INDEX;
}

