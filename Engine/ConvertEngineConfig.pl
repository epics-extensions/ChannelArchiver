#!/usr/bin/perl
# ConvertEngineConfig.pl
#
# kasemir@lanl.gov

use English;
#use strict;
use vars qw($opt_d);
use Getopt::Std;
use File::Basename;

sub usage()
{
    print("USAGE: ConvertEngineConfig [-d DTD] old-config new-config\n");
    print("\n");
    print("This tool reads an old-type ArchiveEngine ASCII configuration\n");
    print("file and converts it into the current XML config file.\n");
    exit(-1);
}

if (!getopts('d:')  ||  $#ARGV != 1)
{
    usage();
}
my ($infile) = $ARGV[0];
my ($outfile) = $ARGV[1];
unless (length($opt_d) > 0)
{
    $opt_d = "engineconfig.dtd";
    printf ("\nUsing '$opt_d' as the DTD\n");
    printf ("You should use the '-d DTD' option\n");
    printf ("to provide the path to your DTD.\n\n");
}
my (%params, %groups, $directory, $filesize_warning);
# Defaults for the global options
%params = 
(
 write_period => 30,
 get_threshold => 20,
 file_size => 30,
 ignored_future => 1.0,
 buffer_reserve => 3,
 max_repeat_count => 120
);
$filesize_warning = 0;
$directory = dirname($infile);
$infile = basename($infile);
parse($infile,'fh00');
dump_xml();

# parse(<group file name>, file handle to use)
# creates something like this:
# %groups =
# (
#  "excas" =>
#  {
#      "fred" => { period => 5, scan => 1 },
#      "janet" => { period => 1, monitor => 1, disable => 1 },
#  },
# );
#
# file handle is used to that parse() can recurse.
# 'recurse': See 'recurse'
sub parse($$)
{
    my ($group,$fh) = @ARG;
    my ($channel, $period, $options, $opt, $monitor, $disable);
    $fh++;
    if (not open $fh, "$group")
    {
	open $fh, "$directory/$group"
	    or die "Cannot open $group nor $directory/$group\n";
    }
    $group = basename($group);
    while (<$fh>)
    {
	chomp;
	next if (m'\A#'); # Skip comments
	next if (m'\A\s*\Z'); # Skip empty lines
	if (m'!(\S+)\s+(\S+)')
	{   # Parameter "!<text> <text>" ?
	    if ($1 eq "group")
	    {
		parse($2,$fh);
	    }
	    elsif (length($params{$1}) > 0)
	    {
		$params{$1} = $2;
		if ($1 eq "file_size")
		{
		    $params{$1} = $2/24 * 10;
		    if (not $filesize_warning)
		    {
			printf("%s, line %d:\n", $group, $NR);
			printf("\tThe 'file_size' parameter used to be in hours,\n");
			printf("\tbut has been changed to MB.\n");
			printf("\tThe automated conversion is arbitrary, assuming a\n");
			printf("\tdata file size of about 10 MB per day (24h).\n");
			$filesize_warning = 1;
		    }
		}
	    }
	    else
	    {
		printf("%s, line %d: Unknown parameter/value '%s' - '%s'\n",
		       $group, $NR, $1, $2);
		exit(-2);
	    }
	}
	elsif (m'\A\s*(\S+)\s+([0-9\.]+)(\s+.+)?\Z')
	{   # <channel> <period> [Monitor|Disable]*
	    $channel = $1;
	    $period = $2;
	    $options = $3;
	    $monitor = $disable = 0;
	    if (not defined($period)  or  $period <= 0)
	    {
		printf("%s, line %d: Invalid scan period\n",
		       $group, $NR);
		exit(-3);
	    }
	    if (length($options) > 0)
	    {
		foreach $opt ( split /\s+/, $options )
		{
		    if ($opt =~ m'\A[Mm]onitor\Z')
		    {
			$monitor = 1;
		    }
		    elsif ($opt =~ m'\A[Dd]isable\Z')
		    {
			$disable = 1;
		    }
		    elsif (length($opt) > 0)
		    {
			printf("%s, line %d: Invalid option '%s'\n",
			       $group, $NR, $opt);
			exit(-4);
		    }
		}
	    }
	    $groups{$group}{$channel}{period} = $period;
	    if ($monitor)
	    {
		$groups{$group}{$channel}{monitor} = $monitor;
	    }
	    else
	    {
		$groups{$group}{$channel}{scan} = 1;
	    }
	    $groups{$group}{$channel}{disable} = $disable;
	}
	else
	{
	    printf("%s, line %d: '%s' is neither comment, option nor channel definition\n",
		   $group, $NR, $_);
	    exit(-5);
	}
    }
    close($fh);
}

sub dump_xml()
{
    my ($parm, $group, $channel);
    open OUT, ">$outfile" or die "Cannot create $outfile\n";
    printf OUT ("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    printf OUT ("<!DOCTYPE engineconfig SYSTEM \"$opt_d\">\n");
    printf OUT ("<engineconfig>\n");
    foreach $parm ( sort keys %params )
    {
	printf OUT ("\t<$parm>$params{$parm}</$parm>\n");
    }
    
    foreach $group ( sort keys %groups )
    {
	printf OUT ("\t<group>\n");
	printf OUT ("\t\t<name>$group</name>\n");
	foreach $channel ( sort keys %{$groups{$group}} )
	{
	    printf OUT ("\t\t<channel><name>$channel</name>");
	    printf OUT ("<period>$groups{$group}{$channel}{period}</period>");
	    printf OUT ("<monitor/>") if ($groups{$group}{$channel}{monitor});
	    printf OUT ("<scan/>") if ($groups{$group}{$channel}{scan});
	    printf OUT ("<disable/>") if ($groups{$group}{$channel}{disable});
	    printf OUT ("</channel>\n");
	}
	printf OUT ("\t</group>\n");
    }
    printf OUT ("</engineconfig>\n");
}
