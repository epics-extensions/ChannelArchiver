#!/usr/bin/perl

use English;
use strict;
use File::Find;
use Time::Local;
use vars qw($opt_h $opt_d $opt_n);
use Getopt::Std;

# Globals -------------------------------------------------------------
# Path names to all the sub-archives,
# used by find()/locate_sub_archive_paths
my (@sub_archive_paths);

# $time = 2004/07/20 00:00:00
# $archives{$time}->{path} = "path/2004/07_20";
my (%archives);

# Name of new index
my ($new_index);

sub usage()
{
    print("USAGE: make_compress_script.pl [options] {path}\n");
    print("\n");
    print("Options:\n");
    print("\t-n <path/index>: full path to new index\n");
    print("\t-h             : help\n");
    print("\t-d             : debug mode\n");
    print("\n");
    print(" {path}          : one or more paths to index files\n");
    print("\n");
    print("This script creates an example shell script for\n");
    print("invoking the ArchiveDataTool in order to combine\n");
    print("daily or weekly sub-archives into bigger ones.\n");
    print("It is usually advisable to check the resulting\n");
    print("scipt before running it.\n");
    exit(-1);
}

sub locate_sub_archive_paths()
{
    my ($index) = $_;
    return unless ($index eq "index");
    print("Found index in '$File::Find::dir'\n") if ($opt_d);
    push @sub_archive_paths, $File::Find::dir;
}

sub analyze_sub_archives()
{
    my ($subarchive, $time);
    foreach $subarchive ( sort @sub_archive_paths )
    {
	if ($subarchive =~ '(.*/)?([0-9]{4})/([0-9]{2})_([0-9]{2})\Z')
	{   # Format path/YYYY/MM_DD
	    $time = timelocal(0, 0, 0, $4, $3, $2);
	}
	elsif ($subarchive =~ '(.*/)?([0-9]{4})/([0-9]{2})_([0-9]{2})_([0-9]{2})\Z')
	{   # Format path/YYYY/MM_DD_HH
	    $time = timelocal(0, 0, $5, $4, $3, $2);
	}
	else
	{
	    die "Cannot decode the format of the sub-archive in '".
		$subarchive . "'\n";
	}
	die "Is $subarchive a duplicate?" if (exists($archives{$time}));
	$archives{$time}->{path} = $subarchive;
    }
}

sub time2txt($)
{
    my ($time) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	= localtime($time);
    return sprintf("%02d/%02d/%04d %02d:%02d:%02d",
		   $mon, $mday, 1900+$year, $hour, $min, $sec);
}

sub create_script()
{
    my (@times) = sort { $a <=> $b } keys %archives;
    my ($N) = $#times + 1;
    my ($i, $t0, $t1, $start, $end, $diff, $prev_diff);
    $prev_diff = 0;

    die "No archives found\n" if ($N < 1);

    if (length($new_index) <= 0)
    {
	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	    = localtime($times[0]);
	$new_index = sprintf("%04d/%02d/index", 1900+$year, $mon);
    }
    print "# Combine the following $N sub-archives into $new_index:\n";
    for ($i=0; $i<$N; ++$i)
    {
	$t0 = $times[$i];
	print "# $archives{$t0}->{path}/index\n";
    }
    print "#\n";
    print "# Check that $new_index can be created, i.e. the path exists!\n";
    print "mkdir -p `dirname $new_index`\n";
    print "\n";
    for ($i=0; $i<$N; ++$i)
    {
	$t0 = $times[$i];
	$start = time2txt($t0);
	if ($i < $N-1)
	{
	    $t1 = $times[$i+1];
	    $end   = time2txt($t1);
	    $diff = $t1 - $t0;
	}
	else
	{
	    $end = undef;
	    $diff = undef;
	}
	print "ArchiveDataTool $archives{$t0}->{path}/index ";
	print "-c $new_index ";
	if (defined($end))
	{
	    print "-s '$start' -e '$end'\n";
	}
	else
	{
	    print "-s '$start'\n";
	}
	if ($prev_diff != 0  and $diff != $prev_diff)
	{
	    print "# NOTE: Interval changed!\n";
	}
	$prev_diff = $diff;
    }
    print "#\n";
}

# ----------------------------------------------------------------
# Main
# ----------------------------------------------------------------

if (!getopts('hdn:')  ||  $#ARGV < 0  ||  $opt_h)
{
    usage();
}

find(\&locate_sub_archive_paths, @ARGV);
analyze_sub_archives();
$new_index = $opt_n if (length($opt_n));
create_script();
