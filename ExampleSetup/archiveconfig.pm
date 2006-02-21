package archiveconfig;
# Perl module to read the file which lists all the daemons and engines
# that we indent to handle.
#
# parse_config_file creates an array of daemons to run,
# their info, and their engines, similar to this:
# 
# $daemons[0]->{name} = "demosys";     # Daemon name (directory)
# $daemons[0]->{desc} = "Test daemon"; # .. description
# $daemons[0]->{port} = 4000;          # .. port
# .. and the engines under this daemon w/ their name, desc, port, ...
# $daemons[0]->{engines}[0]->{name} = "engine1";
# $daemons[0]->{engines}[0]->{desc} = "Test Engine 1";
# $daemons[0]->{engines}[0]->{port} = 4001;
# $daemons[0]->{engines}[0]->{restart} = "daily";
# $daemons[0]->{engines}[0]->{time} = "08:00";
#
# update_status adds the following info:
# $daemons[0]->{running} = true/false;
# $daemons[0]->{engines}[0]->{status} = "running", "disabled", "down";
# $daemons[0]->{engines}[0]->{channels} = 0...
# $daemons[0]->{engines}[0]->{connected} = 0...
# kasemirk@ornl.gov

require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw(parse_config_file dump_config read_URL update_status);

use English;
use strict;
use Socket;
use IO::Handle;
use Sys::Hostname;
use Data::Dumper;
# Linux and MacOSX seem to include this one per default:
use LWP::Simple;
use XML::Simple;

# Timeout used when reading a HTTP client or ArchiveEngine.
# 10 seconds is reasonable.
my ($read_timeout) = 30;

my ($localhost) = hostname();

# Parse the old-style tab separated file format.
#
# The file is supposed to be a TAB-delimited table of the following
# columns:
#
# Type, Name, Port, Description, Time, Frequency
# ----------------------------------------------
# Type:        'DAEMON' or 'ENGINE'
# Name:        Name of daemon or engine subdir
# Port:        TCP port used by daemon or engine
# Description: Any text. Defaults to name it left empty.
# Time:        Time in HH:MM format for the engine restarts
# Frequency:   'daily' or 'weekly' or ...
#
# Returns @daemons
sub parse_tabbed_file($$)
{
    my ($filename, $opt_d) = @ARG;
    my (@daemons);
    my ($in);
    my ($type, $name, $port, $desc, $time, $restart);
    my ($di, $ei); # Index of current daemon and engine

    print("Parsing '$filename' as tabbed file.\n") if ($opt_d);
    open($in, $filename) or die "Cannot open '$filename'\n";
    print("Reading $filename\n") if ($opt_d);
    while (<$in>)
    {
        chomp;                          # Chop CR/LF
        next if ($ARG =~ '\A#');        # Skip comments
        next if ($ARG =~ '\A[ \t]*\Z'); # ... and empty lines
        ($type,$name,$port,$desc,$restart,$time) = split(/\t/, $ARG); # Get columns
        $desc = $name unless (length($desc) > 0); # Desc defaults to name
        if ($type eq "DAEMON")
        {
            print("$NR: Daemon '$name', Port $port, Desc '$desc'\n")
            if ($opt_d);
            $di = $#daemons + 1;
            $daemons[$di]->{name} = $name;
            $daemons[$di]->{desc} = $desc;
            $daemons[$di]->{port} = $port;
            $daemons[$di]->{running} = 0;
            $daemons[$di]->{disabled} = 0;
            $daemons[$di]->{channels} = 0;
            $daemons[$di]->{connected} = 0;
            $ei = 0;
        }
        elsif ($type eq "ENGINE")
        {
            print("$NR: Engine '$name', Port $port, Desc '$desc', Time '$time', Restart '$restart'\n") if ($opt_d);
            $daemons[$di]->{engines}[$ei]->{name} = $name;
            $daemons[$di]->{engines}[$ei]->{desc} = $desc;
            $daemons[$di]->{engines}[$ei]->{port} = $port;
            $daemons[$di]->{engines}[$ei]->{time} = $time;
            $daemons[$di]->{engines}[$ei]->{restart} = $restart;
            ++ $ei;
        }
        else
        {
            die("File '$filename', line $NR: Cannot handle type '$type'\n");
        }
    }
    close $in;
    print("Read $filename\n\n") if ($opt_d);
    return @daemons;
}

sub parse_config_file($$)
{
    my ($filename, $opt_d) = @ARG;

    print("Parsing '$filename'\n") if ($opt_d);
    return parse_tabbed_file($filename, $opt_d) if ($filename =~ m".csv\Z");

    my ($config) = XMLin($filename, ForceArray => [ 'daemon', 'engine' ], KeyAttr => "directory");
    # print Dumper($config);

    my (@daemons);
    my ($d_dir, $e_dir);
    my ($di, $ei); # Index of current daemon and engine
    foreach $d_dir ( sort keys %{ $config->{daemon} } )
    {
            $di = $#daemons + 1;
            $daemons[$di]->{name} = $d_dir;
            $daemons[$di]->{desc} = $config->{daemon}{$d_dir}{description};
            $daemons[$di]->{port} = $config->{daemon}{$d_dir}{port};
            $daemons[$di]->{running} = 0;
            $daemons[$di]->{disabled} = 0;
            $daemons[$di]->{channels} = 0;
            $daemons[$di]->{connected} = 0;
            print("Daemon '$daemons[$di]->{name}', Port $daemons[$di]->{port}, Desc '$daemons[$di]->{desc}'\n")
                if ($opt_d);
            $ei = 0;
            foreach $e_dir ( sort keys %{ $config->{daemon}{$d_dir}{engine} } )
            {
                $daemons[$di]->{engines}[$ei]->{name} = $e_dir;
                $daemons[$di]->{engines}[$ei]->{desc} = $config->{daemon}{$d_dir}{engine}{$e_dir}{description};
                $daemons[$di]->{engines}[$ei]->{port} = $config->{daemon}{$d_dir}{engine}{$e_dir}{port};
                $daemons[$di]->{engines}[$ei]->{restart} = $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type};
                $daemons[$di]->{engines}[$ei]->{time} = $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content};
                print("-- Engine '$daemons[$di]->{engines}[$ei]->{name}', Port $daemons[$di]->{engines}[$ei]->{port}, Desc '$daemons[$di]->{engines}[$ei]->{desc}', Time '$daemons[$di]->{engines}[$ei]->{time}$daemons[$di]->{engines}[$ei]->{time}', Restart '$daemons[$di]->{engines}[$ei]->{restart}'\n") if ($opt_d);
                ++ $ei;
            }
    }
    return @daemons;
}

# Input: Reference to @daemons
sub dump_config($)
{
    my ($daemons) = @ARG;
    my ($daemon, $engine);
    print("Configuration Dump:\n");
    foreach $daemon ( @{ $daemons } )
    {
        printf("Daemon '%s': Port %d, description '%s'\n",
               $daemon->{name},  $daemon->{port},
               $daemon->{desc});
        foreach $engine ( @{ $daemon->{engines} } )
        {
            printf("    Engine '%s', port %d, description '%s', ",
              $engine->{name}, $engine->{port}, $engine->{desc});
            printf("restart %s %s\n",
               $engine->{time}, $engine->{restart});
        }
    }
    print("\n");
}

# Connects to HTTPD at host/port and reads a URL,
# returning the raw document.
sub read_URL($$$)
{
    my ($host, $port, $url) = @ARG;
    my ($content, @doc);
    $content = get("http://$host:$port/$url");
    @doc = split /[\r\n]+/, $content;
    return @doc;
}

# Input: Reference to @daemons, $debug
sub update_status($$)
{
    my ($daemons, $opt_d) = @ARG;
    my (@html, $line, $daemon, $engine);
    foreach $daemon ( @{ $daemons } )
    {
        # Assume the worst until we learn more
        $daemon->{running} = 0;
        foreach $engine ( @{ $daemon->{engines} } )
        {
            $engine->{status} = "unknown";
            $engine->{connected} = 0;
            $engine->{channels} = 0;
        }
        @html = read_URL($localhost, $daemon->{port}, "status");
        if ($opt_d)
        {
            print "Response from $daemon->{desc}:\n";
            foreach $line ( @html )
            {   print "    '$line'\n"; }
        }
        foreach $line ( @html )
        {
            if ($line =~ m"\AENGINE ([^|]*)\|([0-9]+)\|([^|]+)\|([0-9]+)\|([0-9]+)")
            {
                my ($port, $status, $connected, $channels) = ($2, $3, $4, $5);
                if ($opt_d)
                {
                    print("Engine: port $port, status $status, $connected/$channels connected\n");
                }
                $daemon->{running} = 1;
                foreach $engine ( @{ $daemon->{engines} } )
                {
                    if ($engine->{port} == $port)
                    {
                        $engine->{status} = $status;
                        $engine->{connected} = $connected;
                        $engine->{channels} = $channels;
                        last;
                    }
                }
            }
        }
    }
}


