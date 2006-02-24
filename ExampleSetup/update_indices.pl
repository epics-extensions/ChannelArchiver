#!/usr/bin/perl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_s $opt_r);
use Cwd;
use File::Path;
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($dtd_root)    = "/arch";
my ($index_dtd);
my ($daemon_dtd);
my ($engine_dtd);
my ($hostname)    = hostname();
my ($path) = cwd();

sub usage()
{
    print("USAGE: update_indices [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -s <system> : Handle only the given system daemon, not all daemons.\n");
    print("               (Regular expression for daemon name)\n");
    print(" -r <root>   : Use given root for DTD files instead of $dtd_root\n");
    print(" -d          : debug\n");
}

# Configuration info filled by parse_config_file
my ($config);

# Create index.xml in current dir with given @indices.
sub create_indexconfig(@)
{
    my (@indices) = @ARG;
    my ($index);
    my ($name) = "index.xml";
    open(OUT, ">$name") or die "Cannot create " . cwd() . "/$name";
    my ($old_fd) = select OUT;

    print "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    print "<!DOCTYPE indexconfig SYSTEM \"$index_dtd\">\n";
    print "<!--\n";
    print "     Auto-created. Do not edit!\n";
    print "  -->\n";
    print "<indexconfig>\n";
    foreach $index ( @indices )
    {
        print "  <archive>\n";
        print "    <index>$index</index>\n";
        print "  </archive>\n";
    }
    print "</indexconfig>\n";

    select $old_fd;
    close OUT;
}

# Create the index.xml for the given engine directory.
# Looks for all files <year>/<day_or_time>/index.
sub make_engine_index($)
{
    my ($dir) = @ARG;
    chdir($dir);
    my (@indices) = <*/*/index>;
    if ($#indices >= 0)
    {
        create_indexconfig(@indices);
    }
    chdir($path);
}

# Create the index.xml for the given daemon directory.
#
sub make_daemon_index($)
{
    my ($dir) = @ARG;
    chdir($dir);
    my (@indices) = <*/index>;
    if ($#indices >= 0)
    {
        create_indexconfig(@indices);
    }
    chdir($path);
}

# Create the daemon and engine directories
sub create_stuff()
{
    my ($d_dir, $e_dir);
    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
	# Skip daemons/systems that don't match the supplied reg.ex.
	next if (length($opt_s) > 0 and not $d_dir =~ $opt_s);

	print("Daemon $d_dir\n");
	# Daemon Directory
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
	{
	    print(" - Engine $e_dir\n");
	    # Engine and ASCII-config dir
            make_engine_index("$d_dir/$e_dir");
	}
        make_daemon_index($d_dir);
    }
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:s:r:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
$dtd_root    = $opt_r if (length($opt_r) > 0);
$index_dtd   = "$dtd_root/indexconfig.dtd";
$daemon_dtd  = "$dtd_root/ArchiveDaemon.dtd";
$engine_dtd  = "$dtd_root/engineconfig.dtd";

$config = parse_config_file($config_name, $opt_d);
create_stuff();
