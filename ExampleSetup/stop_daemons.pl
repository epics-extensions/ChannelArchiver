#!/usr/bin/perl
# make_archive_infofile.pl

use lib '/arch/scripts';

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_p $opt_o $opt_e);
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.csv";
my ($output_name) = "errors";
my ($localhost) = hostname();

# Configuration info filled by parse_config_file
my (@daemons);

sub usage()
{
    print("USAGE: stop_daemons.pl [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -p          : postal (kill engines as well)\n");
    print(" -d          : debug\n");
}


# The main code ==============================================

# Parse command-line options
if (!getopts("dhpc:o:e") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c  if (length($opt_c) > 0);

@daemons = parse_config_file($config_name, $opt_d);
my ($stop, $daemon, $engine, $quit, $line, @html);

$stop = "stop";
$stop = "postal" if ($opt_p);

foreach $daemon ( @daemons )
{
     printf("Stopping Daemon '%s' on port %d: ",
            $daemon->{name}, $daemon->{port});
     $quit = 0;
     @html = read_URL($localhost, $daemon->{port}, $stop);
     foreach $line ( @html )
     {
         if ($line =~ "Quitting")
         {
             $quit = 1;
             last;
         }
     }
     if ($quit)
     {
         print("Quit.\n");
     }
     else
     {
         print ("Didn't quit. Response:\n");
         foreach $line ( @html )
         {
             print("'$line'\n");
         }
     }
}

