#!/usr/bin/perl
# Dump data in MatLab format

use English;

# Set search path for the Perl module (casi.pm)
use lib "O." . $ENV{HOST_ARCH};
use casi;
#use strict;
# use vars qw($a $c $v $name);
use Getopt::Std;

sub usage()
{
    print("USAGE: $PROGRAM_NAME [options] { channel names }\n");
    print("       -a <archive>\n");
    print("       -s <start time>  - Format: YYYY/MM/DD HH:MM:SS(.SS)\n");
    print("       -e <end time>\n");
    exit;
}

getopts('ha:s:e:');
usage() if $opt_h;
$opt_a = "../../Engine/Test/freq_directory" unless $opt_a;

# Create archive, channel, value, open archive
$a = archive::new();
$c = channel::new();
$v = value::new();
$a->open($opt_a);

print "% MatLab data file, created by $PROGRAM_NAME script.\n";
print "% Data from archive $opt_a\n";
if ($#ARGV > 0)
{
    print "% Channels: " . join(',', @ARGV) . "\n";
}
else
{
    print "% Channel: " . $ARGV[0] . "\n";
}
print "%\n";
print "% Struct: t - time string\n";
print "%         v - value\n";
print "%         d - date number\n";
print "%         l - length of data\n";
print "%         n - name\n";
print "%\n";
print "% kasemir\@lanl.gov\n";
print "\n";

# Dump values for single channel
foreach $name ( @ARGV )
{
    $a->findChannelByName($name, $c)
	or die "Cannot find channel $name\n";
    if ($start)
    {
	$c->getValueAfterTime($start, $v);
    }
    else
    {
	$c->getFirstValue($v);
    }
    
    $i=1;
    while ($v->valid()  and  (!$end  or $v->time() le $end))
    {
	print "$name.t($i)={'" . stamp2datestr($v->time()) . "'};\n";
	if ($v->isInfo())
	{
	    print "$name.v($i)=nan;\n";
	}
	else
	{
	    print "$name.v($i)=" . $v->text() . ";\n";
	}
	++ $i;
	$v->next();
    }
    print "$name.d=datenum(char($name.t));\n";
    print "$name.l=size($name.v,2);\n";
    print "$name.n='$name';\n";
}


# Convert archiver time stamp into MatLab date string
# stamp:    "YYYY/MM/DD hh:mm:ss"
#           with 24h hours and (maybe) fractional seconds
#           up to the nanosecond level.
# date str: MM-DD-YYYY hh.mm.ss
sub stamp2datestr($)
{
    my ($stamp) = @ARG;
    my ($YYYY, $MM, $DD, $hh, $mm, $ss);

    if ($stamp =~ m'(\d{4})/(\d{2})/(\d{2}) (\d{2}):(\d{2}):([0-9.]*)')
    {
	$YYYY = $1;
	$MM   = $2;
	$DD   = $3;
	$hh   = $4;
	$mm   = $5;
	$ss   = $6;
    }
    elsif ($stamp =~ m'(\d{4})/(\d{2})/(\d{2})')
    {
	$YYYY = $1;
	$MM   = $2;
	$DD   = $3;
	$hh   = 0;
	$mm   = 0;
	$ss   = 0;
    }
    else
    {
	die "Invalid time stamp '$stamp'\n";
    }

    return sprintf("%02d-%02d-%04d %02d:%02d:%012.9f",
		   $MM, $DD, $YYYY, $hh, $mm, $ss);
}

