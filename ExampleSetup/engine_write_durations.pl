#!/usr/bin/perl
# engine_write_durations.pl
#
# Query all engines for their "Write Duration"

use lib '/arch/scripts';
use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_o);
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "/arch/archiveconfig.csv";
my ($read_timeout) = 30;
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

my ($daemon, $engine, @html, $line, $time);
foreach $daemon ( @daemons )
{
    foreach $engine ( @{ $daemon->{engines} } )
    {
	printf("    Engine '%s', %s:%d, description '%s'\n",
	       $engine->{name}, $localhost, $engine->{port}, $engine->{desc}) if ($opt_d);
	$time = "<unknown>";
	@html = read_URL($localhost, $engine->{port}, "/");
	foreach $line ( @html )
	{
	    if ($line =~ m"Write Duration.*>([0-9.]+ sec)<")
	    {
		$time = $1;
		last;
	    }
	}
	print "$engine->{name}\t$time\n";
    }
}

