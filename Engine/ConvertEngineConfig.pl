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
    print("This tool reads an old-type ArchiveEngine ASCII config.\n");
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

#%groups =
#(
# "excas" =>
# {
#     "fred" => { period => 5, scan => 1 },
#     "janet" => { period => 1, monitor => 1, disable => 1 },
# },
#);
sub parse($$)
{
    my ($group,$fh) = @ARG;
    my ($channel, $period, $options);
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
	{   # Parameter?
	    if ($1 eq "group")
	    {
		parse($2,$fh);
	    }
	    if (length($params{$1}))
	    {
		$params{$1} = $2;
		if ($1 eq "file_size")
		{
		    $params{$1} = $2/24 * 10;
		    if (not $filesize_warning)
		    {
			printf("The 'file_size' parameter used to be in hours,\n");
			printf("but has been changed to MB.\n");
			printf("The automated conversion is arbitrary, assuming a\n");
			printf("data file size of about 10 MB per day (24h).\n");
			$filesize_warning = 1;
		    }
		}
	    }
	    next;
	}
	if (m'(\S+)\s+([0-9\.]+)?(.*)')
	{
	    $channel = $1;
	    $period = $2;
	    $options = $3;
	    $groups{$group}{$channel}{period} = $period;
	    if ($options =~ m'[Mm]onitor')
	    {
		$groups{$group}{$channel}{monitor} = 1;
	    }
	    else
	    {
		$groups{$group}{$channel}{scan} = 1;
	    }
	    if ($options =~ m'[Dd]isable')
	    {
		$groups{$group}{$channel}{disable} = 1;
	    }
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
