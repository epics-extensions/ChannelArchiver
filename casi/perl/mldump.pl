#!/usr/bin/perl
# Dump data in MatLab format

use English;

# Set search path for the Perl module (casi.pm)
use lib "O." . $ENV{EPICS_HOST_ARCH};
use casi;
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
$opt_a = "../../Engine/Test/index" unless $opt_a;

# Create archive, channel, value, open archive
$a = casi::new_archive();
$c = casi::new_channel();
$v = casi::new_value();
casi::archive_open($a, $opt_a);

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
    casi::archive_findChannelByName($a, $name, $c)
	or die "Cannot find channel $name\n";
    if ($start)
    {
	casi::channel_getValueAfterTime($c, $start, $v);
    }
    else
    {
	casi::channel_getFirstValue($c, $v);
    }
    
    $i=1;
    while (casi::value_valid($v)  and
	   (!$end  or casi::value_time($v) le $end))
    {
	print("$name.t($i)={'" .
	      stamp2datestr(casi::value_time($v)) . "'};\n");
	if (casi::value_isInfo($v))
	{
	    print "$name.v($i)=nan;\n";
	}
	else
	{
	    print "$name.v($i)=" . casi::value_text($v) . ";\n";
	}
	++ $i;
	casi::value_next($v);
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

