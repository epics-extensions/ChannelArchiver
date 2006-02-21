#!/usr/bin/perl
# make_archive_infofile.pl

use lib '/arch/scripts';

use English;
use strict;
use vars qw($opt_h $opt_c $opt_d);
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.csv";

# Configuration info filled by parse_config_file
my (@daemons);

sub usage()
{
    print("USAGE: convert_archiveconfig_to_xml.pl [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -d          : debug\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
}

# Input: Reference to @daemons
sub dump_config_as_xml($)
{
    my ($daemons) = @ARG;
    my ($daemon, $engine);
    print("<!-- Archive Configuration\n");
    print("\n");
    print("     There is no DTD (yet),\n");
    print("     so try to avoid errors when editing this file.\n");
    print("\n");
    print("  -->\n");
    print("<archive_config>\n");
    foreach $daemon ( @{ $daemons } )
    {
        printf("\n    <daemon directory='%s'>\n",
               $daemon->{name});
        printf("        <desc>%s</desc>\n",
               $daemon->{desc});
        printf("        <port>%s</port>\n",
               $daemon->{port});
        foreach $engine ( @{ $daemon->{engines} } )
        {
            printf("        <engine directory='%s'>\n",
              $engine->{name});
            printf("            <desc>%s</desc>\n",
                   $engine->{desc});
            printf("            <port>%s</port>\n",
                   $engine->{port});
            printf("            <restart type='%s'>%s</restart>\n",
               $engine->{restart}, $engine->{time});
            printf("        </engine>\n");
        }
        printf("    </daemon>\n");
    }
    print("</archive_config>\n");
}

# The main code ==============================================

# Parse command-line options
if (!getopts("hc:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c  if (length($opt_c) > 0);

@daemons = parse_config_file($config_name, $opt_d);
dump_config_as_xml(\@daemons);
