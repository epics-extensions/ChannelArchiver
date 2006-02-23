package archiveconfig;
# Perl module to help with the archiveconfig.xml file.
#
# Reads the data as XML::Simple reads the XML file:
# $config->{daemon}{$d_dir}{port}
# $config->{daemon}{$d_dir}{description}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{port}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{description}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content}
#
# and adds some info in update_status:
# $config->{daemon}{$d_dir}{status}
# $config->{daemon}{$d_dir}{engine}{$e_dir}{status}

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
# Returns $config
sub parse_tabbed_file($$)
{
    my ($filename, $opt_d) = @ARG;
    my ($config);
    my ($in);
    my ($type, $name, $d_dir, $e_dir, $port, $desc, $time, $restart);
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
        if ($type eq "DAEMON")
        {
            $d_dir = $name;
            $desc = "$d_dir Daemon" unless (length($desc) > 0); # Desc default.
            print("$NR: Daemon '$d_dir', Port $port, Desc '$desc'\n") if ($opt_d);
            $config->{daemon}{$d_dir}{description} = $desc;
            $config->{daemon}{$d_dir}{port} = $port;
        }
        elsif ($type eq "ENGINE")
        {
            $e_dir = $name;
            $desc = "$e_dir Engine" unless (length($desc) > 0); # Desc default.
            print("$NR: Engine '$e_dir', Port $port, Desc '$desc', Time '$time', Restart '$restart'\n") if ($opt_d);
            $config->{daemon}{$d_dir}{engine}{$e_dir}{description} = $desc;
            $config->{daemon}{$d_dir}{engine}{$e_dir}{port} = $port;
            $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type} = $restart;
            $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content} = $time;
        }
        else
        {
            die("File '$filename', line $NR: Cannot handle type '$type'\n");
        }
    }
    close $in;
    return $config;
}

# Parse XML or old-style tab separated file format.
#
# Returns $config
sub parse_config_file($$)
{
    my ($filename, $opt_d) = @ARG;
    my ($config);

    print("Parsing '$filename'\n") if ($opt_d);
    if ($filename =~ m".csv\Z")
    {
        $config = parse_tabbed_file($filename, $opt_d)
    }
    else
    {
        $config = XMLin($filename, ForceArray => [ 'daemon', 'engine' ], KeyAttr => "directory");
    }
    if ($opt_d)
    {
        print Dumper($config);
        dump_config($config);
    }
    return $config;
}

# Input: $config
sub dump_config($)
{
    my ($config) = @ARG;
    my ($d_dir, $e_dir);
    print("Configuration Dump:\n");
    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
        printf("Daemon '%s': Port %d, description '%s'\n",
               $d_dir,
               $config->{daemon}{$d_dir}{port},
               $config->{daemon}{$d_dir}{description});
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
        {
            printf("    Engine '%s', port %d, description '%s', ",
                   $e_dir,
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{port},
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{description});
            printf("restart %s %s\n",
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{type},
                   $config->{daemon}{$d_dir}{engine}{$e_dir}{restart}{content});
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

# Input: $config, $debug
sub update_status($$)
{
    my ($config, $opt_d) = @ARG;
    my (@html, $line, $d_dir, $e_dir);
    foreach $d_dir ( keys %{ $config->{daemon} } )
    {
        # Assume the worst until we learn more
        foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
        {
            $config->{daemon}{$d_dir}{engine}{$e_dir}{status} = "unknown";
            $config->{daemon}{$d_dir}{engine}{$e_dir}{channels} = 0;
            $config->{daemon}{$d_dir}{engine}{$e_dir}{connected} = 0;
        }
        # Skip daemon if not supposed to run
        if ($config->{daemon}{$d_dir}{'run'} eq 'false')
        {
            $config->{daemon}{$d_dir}{status} = "not checked";
            next;
        }
        $config->{daemon}{$d_dir}{status} = "no response";
        print "Checking $d_dir daemon on port $config->{daemon}{$d_dir}{port}:\n" if ($opt_d);
        @html = read_URL($localhost, $config->{daemon}{$d_dir}{port}, "status");
        if ($opt_d)
        {
            print "Response from $config->{daemon}{$d_dir}{description}:\n";
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
                $config->{daemon}{$d_dir}{status} = "running";
                foreach $e_dir ( keys %{ $config->{daemon}{$d_dir}{engine} } )
                {
                    if ($config->{daemon}{$d_dir}{engine}{$e_dir}{port} == $port)
                    {
                        $config->{daemon}{$d_dir}{engine}{$e_dir}{status} = $status;
                        $config->{daemon}{$d_dir}{engine}{$e_dir}{channels} = $channels;
                        $config->{daemon}{$d_dir}{engine}{$e_dir}{connected} = $connected;
                        last;
                    }
                }
            }
        }
    }
}


