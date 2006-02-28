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
my ($index_dtd);
my ($indexconfig) = "indexconfig.xml";

# What ArchiveIndexTool to use. "ArchiveIndexTool" works if it's in the path.
my ($ArchiveIndexTool) = "ArchiveIndexTool -v 1";
my ($ArchiveIndexLog)  = "ArchiveIndexTool.log";

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

# Array of index commands to run, filled by make_index* routines
my (@cmd);

# Create the index.xml for the given engine directory.
# Looks for all files <year>/<day_or_time>/index.
sub make_engine_index($$)
{
    my ($d_dir, $e_dir) = @ARG;
    chdir("$d_dir/$e_dir");
    my (@indices) = <*/*/index>;
    create_indexconfig(@indices);
    chdir($config->{root});
    push @cmd, 
        "(cd $d_dir/$e_dir; time $ArchiveIndexTool $indexconfig master_index >$ArchiveIndexLog 2>&1)";
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
    chdir($config->{root});

    push @cmd,
        "(cd $d_dir; time $ArchiveIndexTool $indexconfig master_index >$ArchiveIndexLog 2>&1)";
}

# Create the daemon and engine index files.
sub create_indices()
{
    my ($d_dir, $e_dir, $cmd);
    undef @cmd;
    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
	# Skip daemons/systems that don't match the supplied reg.ex.
	next if (length($opt_s) > 0 and not $d_dir =~ $opt_s);

	print("Daemon $d_dir\n");
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
	{
	    print("- Engine $e_dir\n");
            make_engine_index($d_dir, $e_dir)
                if (exists($config->{daemon}{$d_dir}{engine}{$e_dir}{dataserver})
                    and is_localhost($config->{daemon}{$d_dir}{engine}{$e_dir}{dataserver}{host}));
	}
        make_daemon_index($d_dir)
            if (exists($config->{daemon}{$d_dir}{dataserver})
                and is_localhost($config->{daemon}{$d_dir}{dataserver}{host}));
    }

    print("\nIndex commands:\n");
    foreach $cmd ( @cmd )
    {
        print("$cmd\n");
        system($cmd) unless ($opt_n);
    }
}

# Add a key/name/path to the data server config,
# with check for multiple keys
my (@serverconfig);
my (%keys);
sub add_serverconfig($$$)
{
    my ($key, $name, $path) = @ARG;
    die "Duplicate key $key for $name ($path),\nalready used by $keys{$key}\n"
        if exists($keys{$key});
    push @serverconfig, { key => $key, name => $name, path => $path };
    $keys{$key} = "'$name' ($path)";
}

# Create the data server config
sub create_serverconfig()
{
    my ($d_dir, $e_dir, $index);

    undef @serverconfig;
    undef %keys;

    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
       my ($dc) = $config->{daemon}{$d_dir};
        # Daemon-level index
        if (exists($dc->{dataserver})
            and is_localhost($dc->{dataserver}{host}))
        {
            $index = "master_index";
            $index = "indexconfig.xml"
                if ($dc->{dataserver}{index}{type} eq 'list');
            add_serverconfig($dc->{dataserver}{index}{key},
                             $dc->{dataserver}{index}{content},
                             "$config->{root}/$d_dir/$index");
        }
        foreach $e_dir ( keys %{ $dc->{engine} } )
        {
            my ($ec) = $config->{daemon}{$d_dir}{engine}{$e_dir};
            # Engine-level index
            if (exists($ec->{dataserver})
                and is_localhost($ec->{dataserver}{host}))
            {   # current_index
                if (exists($ec->{dataserver}{current_index}))
                {
                    add_serverconfig($ec->{dataserver}{current_index}{key},
                                     $ec->{dataserver}{current_index}{content},
                                     "$config->{root}/$d_dir/$e_dir/current_index");
                }
                # list or binary index for engine data
                $index = "master_index";
                $index = "indexconfig.xml" 
                    if ($ec->{dataserver}{index}{type} eq 'list');
                add_serverconfig($ec->{dataserver}{index}{key},
                                 $ec->{dataserver}{index}{content},
                                 "$config->{root}/$d_dir/$index");
            }
        }
    }
}

sub print_serverconfig()
{
    my ($i);
    open(OUT, ">$config->{serverconfig}") or die "Cannot write to $config->{serverconfig}\n";
    my ($old_fd) = select OUT;
    print("<?xml version='1.0' encoding='UTF-8'?>\n");
    print("<!DOCTYPE serverconfig SYSTEM 'serverconfig.dtd'>\n");
    print("<!--\n");
    print("  Created by update_indices.pl from $config_name.\n");
    print("  Do not edit manually!\n");
    print("  -->\n");
    print("<serverconfig>\n");

    for ($i=0; $i<=$#serverconfig; ++$i)
    {
        print("  <!-- does not exist\n") unless (-r $serverconfig[$i]{path});
        printf("    <archive>\n" .
               "        <key>%d</key>\n" .
               "        <name>%s</name>\n" .
               "        <path>%s</path>\n" .
               "    </archive>\n",
               $serverconfig[$i]{key},
               $serverconfig[$i]{name},
               $serverconfig[$i]{path});
        print("    -->\n") unless (-r $serverconfig[$i]{path});
    }
    print("</serverconfig>\n");
    select($old_fd);
}

# The main code ==============================================

# TODO: check hostname
# TODO: update-only
# TODO: check list -> list -> list -> binary index
# TODO: list-index for 'all'

# Parse command-line options
if (!getopts("dhc:s:n") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
$config      = parse_config_file($config_name, $opt_d);
$index_dtd   = "$config->{root}/indexconfig.dtd";

die "Should run in '$config->{root}'\n" if (cwd() ne $config->{root});
die "Cannot find $index_dtd\n" unless -r $index_dtd;
create_indices();
create_serverconfig();
print_serverconfig();

