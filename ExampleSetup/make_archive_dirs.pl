#!/usr/bin/perl
# make_archive_dirs.pl
#
# Create archive directories and config files
# based on configuration file.

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
my ($config_name);
my ($dtd_root)    = "/arch";
my ($index_dtd)   = "/arch/indexconfig.dtd";
my ($daemon_dtd)  = "/arch/ArchiveDaemon.dtd";
my ($engine_dtd)  = "/arch/engineconfig.dtd";
my ($hostname)    = hostname();

# Configuration info filled by parse_config_file
my (@daemons);

sub usage()
{
    print("USAGE: make_archive_dirs [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file\n");
    print(" -s <system> : Handle only the given system daemon, not whole config file\n");
    print("               (Regular expression for daemon name)\n");
    print(" -r <root>   : Use given root for DTD files instead of $dtd_root\n");
    print(" -d          : debug\n");
}

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

sub create_stuff()
{
    my ($path) = cwd();
    my ($filename, $daemonfile, $daemon, $engine, $old_fd);
    foreach $daemon ( @daemons )
    {
	if (length($opt_s) > 0)
	{   # Skip daemons/systems that don't match the supplied reg.ex.
	    next unless $daemon->{name} =~ $opt_s;
	}
	$daemonfile = "$daemon->{name}-daemon.xml";

	# Daemon Directory
	mkpath($daemon->{name}, 1);

	# Daemon Start Script
	$filename = "$daemon->{name}/run-daemon.sh";
	if (length($filename) > 0)
	{
	    open OUT, ">$filename" or die "Cannot open $filename\n";
	    $old_fd = select OUT;
	    printf("#!/bin/sh\n");
	    printf("#\n");
	    printf("# Launch the %s daemon (%s)\n",
		   $daemon->{name}, $daemon->{desc});
	    printf("\n");
	    printf("ArchiveDaemon.pl -p %d -i %s -u 20 -f %s\n",   
		   $daemon->{port}, $index_dtd, $daemonfile);
	    select $old_fd;
	    close OUT;
	}

	# Daemon Stop Script
	$filename = "$daemon->{name}/stop-daemon.sh";
	if (length($filename) > 0)
	{
	    open OUT, ">$filename" or die "Cannot open $filename\n";
	    $old_fd = select OUT;
	    printf("#!/bin/sh\n");
	    printf("#\n");
	    printf("# Stop the %s daemon (%s)\n",
		   $daemon->{name}, $daemon->{desc});
	    printf("\n");
	    printf("lynx -dump http://%s:%d/stop\n",
		   $hostname, $daemon->{port});
	    select $old_fd;
	    close OUT;
	}

	# Daemon config file
	$filename = check_filename("$daemon->{name}/$daemonfile");
	if (length($filename) > 0)
	{
	    open OUT, ">$filename" or die "Cannot open $filename\n";
	    $old_fd = select OUT;
	    printf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
	    printf("<!DOCTYPE engines SYSTEM \"%s\">\n",
		   $daemon_dtd);
	    printf("<engines>\n");
	    foreach $engine ( @{ $daemon->{engines} } )
	    {
		printf("  <engine>\n");
		printf("    <desc>%s</desc>\n", $engine->{name});
		printf("    <port>%d</port>\n", $engine->{port}); 
		printf("    <config>%s/%s/%s/%s-group.xml</config>\n",
		       $path, $daemon->{name}, $engine->{name}, $engine->{name});
		printf("    <%s>%s</%s>\n",
		       $engine->{freq}, $engine->{time}, $engine->{freq});
		printf("  </engine>\n");
	    }
	    printf("</engines>\n");
	    select $old_fd;
	    close OUT;
	}

	foreach $engine ( @{ $daemon->{engines} } )
	{
	    mkpath("$daemon->{name}/$engine->{name}/ASCIIConfig", 1);    

	    # Engine Stop Script
	    $filename = "$daemon->{name}/$engine->{name}/stop-engine.sh";
	    if (length($filename) > 0)
	    {
		open OUT, ">$filename" or die "Cannot open $filename\n";
		$old_fd = select OUT;
		printf("#!/bin/sh\n");
		printf("#\n");
		printf("# Stop the %s engine (%s)\n",
		       $engine->{name}, $engine->{desc});
		printf("\n");
		printf("lynx -dump http://%s:%d/stop\n",   
		       $hostname, $engine->{port});
		select $old_fd;
		close OUT;
	    }

	    # ASCIIConfig/convert_example.sh
	    $filename = "$daemon->{name}/$engine->{name}/ASCIIConfig/convert_example.sh";
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
		   $engine_dtd, $engine->{name});
	    print("\n");
	    select $old_fd;
	    close OUT;
	}
    }
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:s:r:") ||  $opt_h)
{
    usage();
    exit(0);
}
if (length($opt_c) <= 0)
{
    print("You must supply the -c <config file> option\n\n");
    usage();
    exit(0);
}

$config_name = $opt_c;
$dtd_root    = $opt_r if (length($opt_r) > 0);
$index_dtd   = "$dtd_root/indexconfig.dtd";
$daemon_dtd  = "$dtd_root/ArchiveDaemon.dtd";
$engine_dtd  = "$dtd_root/engineconfig.dtd";

@daemons = parse_config_file($config_name, $opt_d);
dump_config(\@daemons) if ($opt_d);
create_stuff();
