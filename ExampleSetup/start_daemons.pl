#!/usr/bin/perl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_r);
use Getopt::Std;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($root) = "/arch";
my ($localhost) = hostname();

# Configuration info filled by parse_config_file
my ($config);

sub usage()
{
    print("USAGE: start_daemons.pl [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name.\n");
    print(" -r <dir>    : Use root instead of $root.\n");
    print(" -d          : debug\n");
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:r:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
$root = $opt_r if (length($opt_r) > 0);

$config = parse_config_file($config_name, $opt_d);

my ($d_dir, $cmd);
foreach $d_dir ( sort keys %{ $config->{daemon} } )
{
     next unless is_localhost($config->{daemon}{$d_dir}{'run'});
     $cmd = "cd $root/$d_dir;sh run-daemon.sh\n";
     printf("$cmd\n");
     system($cmd);
}

