#!/usr/bin/perl
# make_archive_infofile.pl

#use lib '/arch/scripts';

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_o $opt_e);
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
    print("USAGE: make_archive_infofile [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -o <html>   : Generate given output file instead of $output_name\n");
    print(" -d          : debug\n");
    print(" -e          : Append to file only if there are errors.\n");
}

sub time_as_text($)
{
    my ($seconds) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	= localtime($seconds);
    return sprintf("%04d/%02d/%02d %02d:%02d:%02d",
		   1900+$year, 1+$mon, $mday, $hour, $min, $sec);
}

sub write_info($$)
{
    my ($filename, $only_errors) = @ARG;
    my ($daemon, $engine, $ok, $out, $disconnected);

    # Check if anything's missing
    if ($only_errors)
    {
	$ok = 1;
      CHECK_DAEMONS:
	foreach $daemon ( @daemons )
	{
	    if (not $daemon->{running})
	    {
		$ok = 0;
		last CHECK_DAEMONS;
	    }
	    foreach $engine ( @{ $daemon->{engines} } )
	    {
		if ($engine->{status} ne "running")
		{
		    $ok = 0;
		    last CHECK_DAEMONS;
		}
	    }
	}
	return if ($ok);
    }
    open($out, ">>$filename") or die "Cannot open '$filename'\n";
    print $out "Archive Status as of ", time_as_text(time), "\n";
    print $out "\n";
    foreach $daemon ( @daemons )
    {
	if ($daemon->{running})
	{
	    print $out "Daemon '$daemon->{name}': running\n"
	}
	else
	{
	    print $out "Daemon '$daemon->{name}': NOT RUNNING\n"
	}
	foreach $engine ( @{ $daemon->{engines} } )
	{
            if ($engine->{status} eq "running")
            {
                $disconnected = $engine->{channels} - $engine->{connected};
                if ($disconnected == 0)
                {
                    print $out "Engine '$engine->{name}': $engine->{channels} channels.\n";
                }
                else
                {
                    print $out "Engine '$engine->{name}': $engine->{channels} channels,  $disconnected disconnected.\n";
                }
            }
            else
            {
                print $out "Engine '$engine->{name}': $engine->{status}\n";
            }  
	}
	print $out "\n";
    }
    close $out;
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:o:e") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c  if (length($opt_c) > 0);
$output_name = $opt_o  if (length($opt_o) > 0);

@daemons = parse_config_file($config_name, $opt_d);
update_status(\@daemons, $opt_d);
write_info($output_name, $opt_e);
