#!/usr/bin/perl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_s $opt_n);
use Cwd;
use File::Path;
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Configuration info filled by parse_config_file
my ($config);
# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($hostname)    = hostname();
my ($path)        = cwd();
my ($index_dtd);
my ($indexconfig) = "indexconfig.xml";

# What ArchiveIndexTool to use. "ArchiveIndexTool" works if it's in the path.
my ($ArchiveIndexTool) = "ArchiveIndexTool -v 1";

my ($ArchiveIndexLog) = "ArchiveIndexTool.log";

sub usage()
{
    print("USAGE: update_indices [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -s <system> : Handle only the given system daemon, not all daemons.\n");
    print("               (Regular expression for daemon name)\n");
    print(" -n          : 'nop', do not run ArchiveIndexTool, only create the config files.\n");
    print(" -d          : debug.\n");
}

# Create index.xml in current dir with given @indices.
sub create_indexconfig(@)
{
    my (@indices) = @ARG;
    my ($index);
    open(OUT, ">$indexconfig") or die "Cannot create " . cwd() . "/$indexconfig";
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
sub make_engine_index($$)
{
    my ($d_dir, $e_dir) = @ARG;
    chdir("$d_dir/$e_dir");
    my (@indices) = <*/*/index>;
    create_indexconfig(@indices);
    chdir($path);
    my ($cmd) = "(cd $d_dir/$e_dir; time $ArchiveIndexTool $indexconfig master_index >$ArchiveIndexLog 2>&1)";
    print("$cmd\n");
    system($cmd) unless ($opt_n);
}

# Create the index.xml for the given daemon directory.
#
sub make_daemon_index($)
{
    my ($d_dir) = @ARG;
    chdir($d_dir);
    my ($e_dir, @indices);
    foreach $e_dir ( sort keys %{ $config->{daemon}{$d_dir}{engine} } )
    {
        push @indices, "$e_dir/master_index";
    }
    create_indexconfig(@indices);
    chdir($path);

    my ($cmd) = "(cd $d_dir; time $ArchiveIndexTool $indexconfig master_index >$ArchiveIndexLog 2>&1)";
    print("$cmd\n");
    system($cmd) unless ($opt_n);
}

# Create the daemon and engine directories
sub create_stuff()
{
    my ($d_dir, $e_dir);
    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
	# Skip daemons/systems that don't match the supplied reg.ex.
	next if (length($opt_s) > 0 and not $d_dir =~ $opt_s);

	print("# Daemon $d_dir\n");
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
	{
	    print("#  - Engine $e_dir\n");
            make_engine_index($d_dir, $e_dir);
	}
        make_daemon_index($d_dir);
    }
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:s:n") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
$config      = parse_config_file($config_name, $opt_d);
$index_dtd   = "$config->{root}/indexconfig.dtd";

die "Cannot find $index_dtd\n" unless -r $index_dtd;
create_stuff();

