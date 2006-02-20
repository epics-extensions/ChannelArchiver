#!/usr/bin/perl
# engine_write_durations.pl
#
# Query all engines for their "Write Duration"

#use lib '/arch/scripts';
use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_o);
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "/arch/archiveconfig.csv";
my ($localhost) = hostname();

# Configuration info filled by parse_config_file
my (@daemons);

sub usage()
{
    print("USAGE: engine_write_durations [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -d          : debug\n");
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:o:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c  if (length($opt_c) > 0);

@daemons = parse_config_file($config_name, $opt_d);

my ($daemon, $engine, @html, $line, $channels, $count, $time, $vps, $period);
my ($total_channels, $total_count, $total_time, $total_vps);
printf "ArchiveEngine 'write' Statistics\n\n";
printf "Engine              Port      Channels  Val.Count Wr.Period Val/sec   Write Duration\n";
printf "------------------------------------------------------------------------------------\n";
$total_channels = 0;
$total_count = 0;
$total_time = 0;
$total_vps = 0;
foreach $daemon ( @daemons )
{
    foreach $engine ( @{ $daemon->{engines} } )
    {
	printf("    Engine '%s', %s:%d, description '%s'\n",
	       $engine->{name}, $localhost, $engine->{port}, $engine->{desc}) if ($opt_d);
	$channels = "<unknown>";
	$count = "<unknown>";
	$time = "<unknown>";
	$period= "<unknown>";
	@html = read_URL($localhost, $engine->{port}, "");
	foreach $line ( @html )
	{
            print "    '$line'\n" if ($opt_d);
	    if ($line =~ m"Channels.*>([0-9.]+)<")
	    {
		$channels = $1;
	    }
	    if ($line =~ m"Write Count.*>([0-9.]+)<")
	    {
		$count = $1;
	    }
	    if ($line =~ m"Write Duration.*>([0-9.]+ sec)<")
	    {
		$time = $1;
	    }
	    if ($line =~ m"Write Period.*>([0-9.]+ sec)<")
	    {
		$period = $1;
		last;
	    }
	}
	if ($count > 0  and  $period > 0)
        {
	    $vps = $count/$period;
        }
        else
        {
	    $vps = 0;
        }
	printf "%-20s%-10d%-10s%-10s%-10s%8.3f  %s\n",
               $engine->{name}, $engine->{port}, $channels, $count, $period, $vps, $time;
	$total_channels += $channels if ($channels > 0);
	$total_count += $count if ($count > 0);
	$total_vps += $vps if ($vps > 0);
	$total_time += $time if ($time > 0);
    }
}
printf "------------------------------------------------------------------------------------\n";
printf "Total:                        %-9d %-9d           %8.3f  %.3f sec\n",
    $total_channels, $total_count, $total_vps, $total_time;
