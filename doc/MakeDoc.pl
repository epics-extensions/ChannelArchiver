#!/usr/bin/perl
#
#	Extracts HTML documentation from source code.
#
#	//CLASS Classname
#	//	Text to copy
#	opens a html file of the same name as the document.
#	The rest of the line is transformed into an HTML header.
#	You may also define the html-filename with
#	//CLASS'filename_without_ending' Classname
#	The following comment lines are copied.
#
#	//*	Start of bulleted method description
#	//	Description goes on..
#	//	Description goes on..
#	int this_prototype_gets_copied ();
#
#	//-	Copy this comment
#	//	Description goes on..
#	//>	Copy this text and next lines

use Getopt::Std;

sub Usage()
{
	print "Usage: $0 [flags] <sourcefile>\n";
	print "\tFlags:\n";
	print "\t-h     : help (you're looking at it)\n";
	print "\t-v     : verbose\n";
}
$ok=getopts('hv');

if ($opt_h or not $ok)
{
	Usage();
	exit;
}

$indexname="index.htm";
$background='BGCOLOR=#F0F0FF BACKGROUND="../blueback.jpg"';

# global:
# @TOC_class
# $file_open
# IFILE, OFILE

$file_open = 0;
$ul_open = 0;
$in_header = 0;
$class_header = 0;

sub OpenFile ($) # filename
{
	my ($filename) =@_;

	CloseFile(1);
	open OFILE, ">$filename" or die "Cannot open $filename";
	select OFILE;
	$file_open = 1;
	$ul_open = 0;

	print <<HEADER;
<HTML>
<HEAD>
<TITLE>Class Documentation</TITLE>
<BODY $background>
<FONT FACE="Comic Sans MS,Arial, Helvetica">
<BLOCKQUOTE>
HEADER
}

sub CloseFile($)
{
	my ($add_indexref) = @_;
	if ($file_open)
	{
		print "</UL>\n" if $ul_open;
		
		print <<INDEX if ($add_indexref);
<HR WIDTH=40% ALIGN=LEFT>
<A HREF="$indexname#Main">Index</A>
INDEX

		print <<FOOT;
<P ALIGN=CENTER>
<FONT SIZE=1>
Created from sources with <I>MakeDoc</I> Perl script.
</FONT>
</BLOCKQUOTE>
</FONT>
</P>
</BODY>
FOOT
		close OFILE;
		select STDOUT;
	}
	$file_open = 0;
}

sub ParseFile ($) # filename
{
	my ($filename) = @_;
	my ($continue, $open_ul, $quote_next, $line, $class);

	print "Parse file '$filename'\n" if $opt_v;
	open IFILE, $filename or die "cannot open $filename";
	$continue=0;
	$quote_next=0;
	$in_quote=0;
	$open_ul=0;

	$filename =~ s'.*/'';
	$filename =~ s'\.[hcp]+$'';
	$filename .= '.htm';
	while (<IFILE>)
	{
		if ($continue)
		{
			if (m'//[ \t]?(.*)')
			{
				if ($1)
				{
					$line = $1;
					$line =~ s/CLASS ([A-Za-z0-9_]+)/<A HREF="$1.htm#Main">$1<\/A>/;
					print "$line\n";
				}
				else
				{
					print "<BR>\n";
				}
				next;
			}
			# else:
			if ($in_header)
			{
				if ($class_header)
				{	print "<H2>Interface</H2>\n"; }
				else
				{	print "<H2>Details</H2>\n"; }
				$in_header = 0;
				$class_header = 0;
				$continue=0;
			}
			if ($quote_next)
			{
				print "<PRE>\n";
				$quote_next=0;
				$in_quote=1;
			}
			if ($in_quote)
			{
				s'^[ \t]?'';
				chomp;
				if ($_)
				{
					print "$_\n";
				}
				else
				{
					print "</PRE>\n";
					$in_quote=0;
					$continue=0;
				}
			}
			next;
		}
		if (m'//CLASS')
		{
			if (m"CLASS'(.+)'[ \t]*(.*)")
			{
				$filename = "$1.htm";
				$class   = $2;
			}
			else
			{
				($class) = /CLASS[ \t]*(.*)/;
				$filename = "$class.htm"
			}
			OpenFile ($filename);
			print "<H1><A NAME=Main>$class Class</A></H1>\n\n";
			push @TOC_class, $class;
			$continue=1;
			$in_header = 1;
			$class_header = 1;
			next;
		}
		if (m'//PACKAGE')
		{
			if (m"PACKAGE'(.+)'[ \t]*(.*)")
			{
				$filename = "$1.htm";
				$package   = $2;
			}
			else
			{
				($package) = /PACKAGE[ \t]*(.*)/;
				$filename = "$package.htm"
			}
			OpenFile ($filename);
			print "<H1><A NAME=Main>Package: $package</A></H1>\n\n";
			push @TOC_class, $package;
			$continue=1;
			$in_header = 1;
			next;
		}
		if (m'//COMMAND')
		{
			if (m"COMMAND'(.+)'[ \t]*(.*)")
			{
				$filename = "$1.htm";
				$command   = $2;
			}
			else
			{
				($command) = /COMMAND[ \t]*(.*)/;
				$filename = "$command.htm"
			}
			OpenFile ($filename);
			print "<H1><A NAME=Main>Command: $command</A></H1>\n\n";
			push @TOC_class, $command;
			$continue=1;
			$in_header = 1;
			next;
		}
		if (m'//\*[ \t]*(.*)')
		{
			if (not $ul_open)
			{
				print "<UL>\n";
				$ul_open = 1;
				}
			print "<LI>\n";
			$line = $1;
			$line =~ s/CLASS ([A-Za-z0-9_]+)/<A HREF="$1.htm#Main">$1<\/A>/;
			print "$line\n";
			$continue=1;
			$quote_next=1;
			next;
		}
		if (m'//>[ \t]*(.*)')
		{
			$line = $1;
			$line =~ s/CLASS ([A-Za-z0-9_]+)/<A HREF="$1.htm#Main">$1<\/A>/;
			print "$line\n" if ($line);
			$continue=1;
			$quote_next=1;
			next;
		}
	}

	CloseFile(1);
}

sub WriteIndex
{
	my ($class);

	OpenFile ($indexname);
	print "<H1><A NAME=Main>Index:</A></H1>\n";
	print "<UL>\n";
	@TOC_class = sort @TOC_class;
	while ($class = shift @TOC_class)
	{
		print "<LI><A HREF=\"$class.htm#Main\">$class</A>\n"
	}
	print "</UL>\n";
	print <<END;
<HR>
END
	CloseFile(0);
}

foreach $pattern ( @ARGV )
{
	foreach $file ( glob $pattern )
	{
		ParseFile($file);
	}
}
WriteIndex();

