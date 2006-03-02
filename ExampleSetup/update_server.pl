#!/usr/bin/perl
#
# Based on Greg Lawson's relocate_arch_data.pl
#
# Reads the mailbox dir, copies new data,
# and re-indexes.
#
# For the copy to work via scp,
# key pairs need to be configured.
# Assume this is the data server computer that
# needs to use scp to pull data off archive computers.
#
# Assume the script runs as user 'xfer'
# and needs to pull data from 'arch1':
#
# Once only:
# ssh-keygen -t dsa
#
# Copy public key over:
# scp ~/.ssh/id_dsa.pub xfer@arch1:~/.ssh/xx
#
# Configure public key on arch1:
# ssh arch1
# cd .ssh/
# cat xx >>authorized_keys 
# rm xx
# chmod 600 authorized_keys 
#
# From now on, you should be able to go to 'arch1'
# without being queried for a pw.

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c);
use Cwd;
use File::Path;
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($path) = cwd();

sub usage()
{
    print("USAGE: update_server [options]\n");
    print("\n");
    print("Reads $config_name, checks <mailbox> directory\n");
    print("for new data.\n");
    print("Copied data meant for this server,\n");
    print("then performs an index update.\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -d          : debug\n");
}

# Configuration info filled by parse_config_file
my ($config);

sub check_mailbox()
{
    my ($updates) = 0;
    my ($entry);
    return 0 unless exists($config->{mailbox});
    chdir($config->{mailbox});
    mkpath("done");
    foreach $entry ( <*> )
    {
        next if ($entry eq 'done');
	print("Mailbox file '$entry':\n") if ($opt_d);
	open(MB, "$entry") or die "Cannot open '$entry'\n";
	while (<MB>)
        {
            chomp;
            print("$_\n") if ($opt_d);
            my ($info, $src, $dst) = split(/\s+/);
            my ($src_host, $src_dir) = split(/:/, $src);
            my ($dst_host, $dst_dir) = split(/:/, $dst);
            if ($info eq "new" and defined($src_host) and is_localhost($src_host))
            {   # Data already here, we need to update
                ++$updates;
            }
            elsif ($info eq "copy" and defined($dst_host) and is_localhost($dst_host))
            {   # Copy data here, then update
                print("mkdir -p $destdir && scp -r $src_host:$src_dir $dst_dir\n");
                ++$updates;
            }
        }
        close(MB);
        # rename
        rename($entry, "done/$entry");
    } 
    return $updates;
} 

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
$config = parse_config_file($config_name, $opt_d);

die "Has to run in $config->{root}\n" unless ($path eq $config->{root});

my ($updates) = check_mailbox();
chdir($path);
if ($updates)
{
    print("perl scripts/update_indices.pl -u\n");
}


