#!/usr/bin/perl

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_s);
use Cwd;
use File::Path;
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($hostname)    = hostname();
my ($path) = cwd();
my ($root, $index_dtd, $daemon_dtd, $engine_dtd);

sub usage()
{
    print("USAGE: update_archive_tree [options]\n");
    print("\n");
    print("Creates or updates archive directory tree,\n");
    print("creates config files for daemons and engines\n");
    print("based on $config_name.\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -s <system> : Handle only the given system daemon, not all daemons.\n");
    print("               (Regular expression for daemon name)\n");
    print(" -d          : debug\n");
}

# Configuration info filled by parse_config_file
my ($config);

sub check_filename($)
{
    my ($filename) = @ARG;
    my ($response);
    if (-f $filename)
    {
	printf("'$filename' already exists.\n");
	printf("[O]verwrite, use [n]ew '$filename.new', or Skip (default) ? >");
	$response = <>;
	return $filename if ($response =~ '\A[oO](ve?)?');
	return "$filename.new" if ($response =~ '\A[nE](ew?)?');
	return "";
    }
    return $filename;
}

# Create scripts for engine
sub create_engine_files($$)
{
    my ($d_dir, $e_dir) = @ARG;
    my ($filename, $daemonfile, $daemon, $engine, $old_fd);

    # Engine Stop Script
    $filename = "$d_dir/$e_dir/stop-engine.sh";
    open OUT, ">$filename" or die "Cannot open $filename\n";
    $old_fd = select OUT;
    printf("#!/bin/sh\n");
    printf("#\n");
    printf("# Stop the %s engine (%s)\n",
           $e_dir,
	   $config->{daemon}{$d_dir}{engine}{$e_dir}{desc});
    printf("\n");
    printf("lynx -dump http://%s:%d/stop\n",   
           $hostname,
	   $config->{daemon}{$d_dir}{engine}{$e_dir}{port});
    select $old_fd;
    close OUT;

    # ASCIIConfig/convert_example.sh
    $filename = "$d_dir/$e_dir/ASCIIConfig/convert_example.sh";
    open OUT, ">$filename" or die "Cannot open $filename\n";
    $old_fd = select OUT;
    printf("#!/bin/sh\n");
    printf("#\n");
    printf("# Example script for creating an XML engine config.\n");
    printf("#\n");
    printf("# THIS ONE WILL BE OVERWRITTEN!\n");
    printf("# Copy to e.g. 'convert.sh' and\n");
    printf("# modify the copy to suit your needs.\n");
    printf("#\n");
    printf("\n");
    printf("echo \"Creating 'example' channel list...\"\n");
    printf("echo \"# Example channel list\"  >example\n");
    printf("echo \"\"                        >>example\n");
    printf("echo \"fred 5\"                  >>example\n");
    printf("echo \"janet 1 Monitor\"         >>example\n");
    printf("\n");
    printf("REQ=\"\"\n");
    printf("REQ=\"\$REQ example\"\n");
    printf("\n");
    printf("echo \"Converting to engine config file...\"\n");
    printf("ConvertEngineConfig.pl -v -d %s -o ../%s-group.xml \$REQ\n",
	   $engine_dtd, $e_dir);
    print("\n");
    select $old_fd;
    close OUT;
}

# Create scripts for daemon
sub create_daemon_files($)
{
    my ($d_dir) = @ARG;
    my ($filename, $daemonfile, $e_dir, $old_fd);
    
    $daemonfile = $d_dir . "-daemon.xml";

    # Daemon config file
    $filename = "$d_dir/$daemonfile";
    open OUT, ">$filename" or die "Cannot open $filename\n";
    $old_fd = select OUT;
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
    printf("<!DOCTYPE engines SYSTEM \"%s\">\n",
	   $daemon_dtd);
    printf("\n");
    printf("<!-- \n");
    printf("  NOTE:\n");
    printf("  This file will be OVERWRITTEN\n");
    printf("  with information from $config_name.\n");
    printf("\n");
    printf("  Change $config_name and re-run\n");
    printf("  update_archive_tree.pl\n");
    printf("  -->\n");
    printf("\n");
    printf("<engines>\n");
    if (is_localhost($config->{daemon}{$d_dir}{run}))
    {
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
        {
	    if (! is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{run}))
	    {
	        printf(" <!-- not running\n");
	    }
	    printf("  <engine>\n");
	    printf("    <desc>%s</desc>\n",
	           $config->{daemon}{$d_dir}{engine}{$e_dir}{desc});
	    printf("    <port>%d</port>\n",
	           $config->{daemon}{$d_dir}{engine}{$e_dir}{port}); 
	    printf("    <config>%s/%s/%s/%s-group.xml</config>\n",
	           $path, $d_dir, $e_dir, $e_dir);
	    if (length($config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type}) > 0)
	    {
	        printf("    <%s>%s</%s>\n",
		       $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type},
		       $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content},
		       $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type});
	    }
	    printf("  </engine>\n");
	    if (! is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{run}))
	    {
	        printf("  :: not running -->\n");
	    }
        }
    }
    else
    {
	printf("<!-- not running on this computer:\n");
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
        {
            printf("  engine '%s' (%s) on port %d\n",
                   $e_dir,
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{desc},
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{port});
        }
	printf("  -->\n");
    }
    printf("</engines>\n");
    select $old_fd;
    close OUT;

    # Daemon Start Script
    $filename = "$d_dir/run-daemon.sh";
    open OUT, ">$filename" or die "Cannot open $filename\n";
    $old_fd = select OUT;
    printf("#!/bin/sh\n");
    printf("#\n");
    if (is_localhost($config->{daemon}{$d_dir}{run}))
    {
	printf("# Launch the %s daemon (%s)\n",
	       $d_dir,
	       $config->{daemon}{$d_dir}{desc});
	printf("\n");
	printf("ArchiveDaemon.pl -p %d -i %s -u 0 -f %s/%s/%s\n",   
	       $config->{daemon}{$d_dir}{port},
	       $index_dtd, $path, $d_dir, $daemonfile);
    }
    else
    {
	printf("echo \"The %s daemon (%s) does not run on this computer.\"\n",
	       $d_dir,
	       $config->{daemon}{$d_dir}{desc});
	printf("\n");
    }
    select $old_fd;
    close OUT;

    # Daemon Stop Script
    $filename = "$d_dir/stop-daemon.sh";
    open OUT, ">$filename" or die "Cannot open $filename\n";
    $old_fd = select OUT;
    printf("#!/bin/sh\n");
    printf("#\n");
    printf("# Stop the %s daemon (%s)\n",
	   $d_dir,
	   $config->{daemon}{$d_dir}{desc});
    printf("\n");
    printf("lynx -dump http://%s:%d/stop\n",
	   $hostname,
	   $config->{daemon}{$d_dir}{port});
    select $old_fd;
    close OUT;
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
	mkpath($d_dir, 1);
	create_daemon_files($d_dir);
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
	{
	    print(" - Engine $e_dir\n");
	    # Engine and ASCII-config dir
	    mkpath("$d_dir/$e_dir/ASCIIConfig", 1);    
	    create_engine_files($d_dir, $e_dir);
	}
    }
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:s:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
$root    = $config->{root};
$index_dtd   = "$root/indexconfig.dtd";
$daemon_dtd  = "$root/ArchiveDaemon.dtd";
$engine_dtd  = "$root/engineconfig.dtd";

$config = parse_config_file($config_name, $opt_d);
create_stuff();
