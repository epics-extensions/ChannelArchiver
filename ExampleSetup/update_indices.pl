#!/usr/bin/perl
#
# See usage()

BEGIN
{
    push(@INC, 'scripts' );
    push(@INC, '/arch/scripts' );
}

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_s $opt_u $opt_n);
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
    print("Based on $config_name, indexconfig.xml files\n");
    print("are created whereever a <dataserver> entry\n");
    print("for the <daemon> or <engine> describes an index.\n");
    print("\n");
    print("For binary indices, the $ArchiveIndexTool is run.\n");
    print("\n");
    print("Finally, the data server configuration is updated.\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -s <system> : Handle only the given system daemon, not all daemons.\n");
    print(" -u          : Update only, no full re-creation, of binary indices.\n");
    print("               Index files older than the master_index are ignored.\n");
    print(" -n          : 'nop', do not run ArchiveIndexTool, only create the config files.\n");
    print(" -d          : debug.\n");
}

# Create index.xml in current dir with given @indices.
sub create_indexconfig($@)
{
    my ($full, @indices) = @ARG;
    my ($index, $index_age);

    # Get age of master index.
    if (-f "master_index")
    {
        $index_age = -M "master_index";
    }
    else
    {   # Need full update if master doesn't exist, yet.
        $full = 1;
    }
    
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
	if ((not $full) and (-M $index) > $index_age)
        {
	    print("    <!-- Older than 'master_index': '$index' -->\n");
        }
        else
        {
            print("    <archive>\n");
            print("        <index>$index</index>\n");
            print("    </archive>\n");
        }
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

    # A 'binary' index might try update-only.
    my ($full) = 0;
    $full = 1 if ($config->{daemon}{$d_dir}{engine}{$e_dir}{dataserver}{index}{type} eq 'list');
    $full = 1 unless $opt_u;
    print "FULL? $d_dir/$e_dir: '$full' '$config->{daemon}{$d_dir}{engine}{$e_dir}{dataserver}{index}{type}' '$opt_u'\n";

    # Collect all sub-archives
    chdir("$d_dir/$e_dir");
    my (@indices) = <*/*/index>;
    # Write
    create_indexconfig($full, @indices);
    chdir($config->{root});
    push @cmd, 
        "(cd $d_dir/$e_dir;(time $ArchiveIndexTool $indexconfig master_index) >$ArchiveIndexLog 2>&1)";
}

# Create the index config for the given daemon directory.
sub make_daemon_index($)
{
    my ($d_dir) = @ARG;
    my ($e_dir, @indices);
    # A 'binary' index might try update-only.
    my ($full) = $config->{daemon}{$d_dir}{dataserver}{index}{type} eq 'list'
                 or not $opt_u;
    foreach $e_dir ( sort keys %{ $config->{daemon}{$d_dir}{engine} } )
    {
	# Engine indices are currently limited to binary -> master_index.
        push @indices, "$e_dir/master_index";
    }
    # Write
    chdir($d_dir);
    create_indexconfig($full, @indices);
    chdir($config->{root});
    
    # Need to create binary daemon index?
    if ($config->{daemon}{$d_dir}{dataserver}{index}{type} eq 'binary')
    {
	push @cmd,
        "(cd $d_dir; time $ArchiveIndexTool $indexconfig master_index >$ArchiveIndexLog 2>&1)";
    }
}

# Add a key/name/path to the data server config,
# with check for duplicate key usage.
my (%serverconfig);
sub add_serverconfig($$$)
{
    my ($key, $name, $path) = @ARG;
    die "Duplicate key $key for $name ($path),\n"
        . "already used by $serverconfig{$key}{name} ($serverconfig{$key}{path})\n"
        if exists($serverconfig{$key});
    $serverconfig{$key} = { name => $name, path => $path };
}

# Print server configuration to $config->{serverconfig},
# sorted by key.
sub print_serverconfig()
{
    open(OUT, ">$config->{serverconfig}") or die "Cannot write to $config->{serverconfig}\n";
    my ($old_fd) = select OUT;
    print("<?xml version='1.0' encoding='UTF-8'?>\n");
    print("<!DOCTYPE serverconfig SYSTEM 'serverconfig.dtd'>\n");
    print("<!--\n");
    print("  Created by update_indices.pl from $config_name.\n");
    print("  Do not edit manually!\n");
    print("  -->\n");
    print("<serverconfig>\n");

    my ($key);
    foreach $key ( sort { $a <=> $b } keys %serverconfig )
    {
        printf("    <archive>\n" .
               "        <key>%d</key>\n" .
               "        <name>%s</name>\n" .
               "        <path>%s</path>\n" .
               "    </archive>\n",
               $key,
               $serverconfig{$key}{name},
               $serverconfig{$key}{path});
    }
    print("</serverconfig>\n");
    select($old_fd);
}

# Create the daemon and engine index files.
sub create_indices()
{
    my ($d_dir, $e_dir, $cmd, $index);
    undef @cmd;
    undef %serverconfig;

    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
	# Skip daemons/systems that don't match the supplied reg.ex.
	next if (length($opt_s) > 0 and not $d_dir =~ $opt_s);

	print("Daemon $d_dir\n");
        my ($dc) = $config->{daemon}{$d_dir};
	my ($need_daemon_index) = 0;
	# Daemon-level index
        if (exists($dc->{dataserver}))
        {   # Check config.
            if (not exists($dc->{dataserver}{index}{key}))
            {
                die "Incomplete <dataserver> entry for '$d_dir':\n" .
                    "No <index> with 'key' attribute.\n";
            }
            # Build on this host?
            if (is_localhost($dc->{dataserver}{host}))
            {
                $index = ($dc->{dataserver}{index}{type} eq 'list')
                         ? "indexconfig.xml" : "master_index";
                add_serverconfig($dc->{dataserver}{index}{key},
                                 $dc->{dataserver}{index}{content},
                                 "$config->{root}/$d_dir/$index");
		$need_daemon_index = 1;
            }
        }
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
	{
	    print("- Engine $e_dir\n");
            my ($ec) = $config->{daemon}{$d_dir}{engine}{$e_dir};
            # Engine-level index
            if (exists($ec->{dataserver}))
            {
                if (is_localhost($ec->{dataserver}{host}))
                {   # current_index
                    if (exists($ec->{dataserver}{current_index}))
                    {
                        add_serverconfig($ec->{dataserver}{current_index}{key},
                                         $ec->{dataserver}{current_index}{content},
                                         "$config->{root}/$d_dir/$e_dir/current_index");
                    }
		    # Check config.
                    if (not exists($ec->{dataserver}{index}{key}))
                    {
                        die "Incomplete <dataserver> entry for '$d_dir/$e_dir':\n" .
                            "No <index> with 'key' attribute.\n";
                    }
                    if ($ec->{dataserver}{index}{type} ne "binary")
                    {
                        die "Invalid <dataserver> entry for '$d_dir/$e_dir':\n" .
                            "Currently only 'binary' supported on engine level.\n";
                    }
                    # list or binary index for engine data
                    add_serverconfig($ec->{dataserver}{index}{key},
                                     $ec->{dataserver}{index}{content},
                                     "$config->{root}/$d_dir/$e_dir/master_index");
		    make_engine_index($d_dir, $e_dir);
                }
            }
	}
	# If daemon index is requested, build it after
	# engine indices have been updated.
	if ($need_daemon_index)
	{
	    make_daemon_index($d_dir);
	}
    }

    print("\nIndex commands:\n");
    foreach $cmd ( @cmd )
    {
        print("$cmd\n");
        system($cmd) unless ($opt_n);
    }
}

# The main code ==============================================

# TODO: update-only
# TODO: check list -> list -> list -> binary index
# TODO: list-index for 'all'

# Parse command-line options
if (!getopts("dhc:s:un") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c if (length($opt_c) > 0);
$config      = parse_config_file($config_name, $opt_d);
$index_dtd   = "$config->{root}/indexconfig.dtd";

die "Should run in '$config->{root}'\n" unless (cwd() eq $config->{root});
die "Cannot find $index_dtd\n" unless -r $index_dtd;
create_indices();
print_serverconfig();

