package archiveconfig;
# Perl module to read the file which lists all the daemons and engines
# that we indent to handle.
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
# See daemon config for details of Time & frequency.
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
@EXPORT = qw(parse_config_file dump_config update_status);

use English;
use strict;
use Socket;
use IO::Handle;
use Sys::Hostname;
use Data::Dumper;

# Timeout used when reading a HTTP client or ArchiveEngine.
# 10 seconds is reasonable.
my ($read_timeout) = 10;

my ($localhost) = hostname();

# Returns @daemons
sub parse_config_file($$)
{
    my ($filename, $opt_d) = @ARG;
    my (@daemons);
    my ($in);
    my ($type, $name, $port, $desc, $time, $restart);
    my ($di, $ei); # Index of current daemon and engine
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
	    print("$NR: Engine '$name', Port $port, Desc '$desc', Time '$time', Restart '$restart'\n")
		if ($opt_d);
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
	    printf("    Engine '%s', port %d, description '%s'\n",
		  $engine->{name}, $engine->{port}, $engine->{desc});
	    printf("     restart %s %s\n",
		   $engine->{time}, $engine->{restart});
	}
    }
    print("\n");
}

# Connects to HTTPD at host/port and reads a URL,
# returning the raw document.
sub read_URL($$$)
{
    my ($host, $port, $URL) = @ARG;
    my ($ip, $addr, $EOL);
    $EOL = "\015\012";
    $ip = inet_aton($host) or die "Invalid host: $host";
    $addr = sockaddr_in($port, $ip);
    socket(SOCK, PF_INET, SOCK_STREAM, getprotobyname('tcp'))
        or die "Cannot create socket: $!\n";
    connect(SOCK, $addr)  or  return "";
    autoflush SOCK 1;
    print SOCK "GET $URL HTTP/1.0$EOL";
    print SOCK "Accept: text/html, text/plain$EOL";
    print SOCK "User-Agent: perl$EOL";
    print SOCK "$EOL";
    print SOCK "$EOL";
    my ($mask, $num, $line, @doc);
    $mask = '';
    vec($mask, fileno(SOCK), 1) = 1;
    $num = select($mask, undef, undef, $read_timeout);
    while ($num > 0  and not eof SOCK)
    {
        $line = <SOCK>;
        push @doc, $line;
        chomp $line;
        $mask = '';
        vec($mask, fileno(SOCK), 1) = 1;
        $num = select($mask, undef, undef, $read_timeout);
    }
    close (SOCK);
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
	    $engine->{status} = "down";
	    $engine->{connected} = 0;
	    $engine->{channels} = 0;
	}
	@html = read_URL($localhost, $daemon->{port}, "/status");
	print "Response from $daemon->{desc}:\n" if ($opt_d);
	print @html if ($opt_d);
	foreach $line ( @html )
	{
	    if ($line =~ m"\AENGINE ([^|]*)\|([0-9]+)\|([^|]+)\|([0-9]+)\|([0-9]+)")
	    {
		$daemon->{running} = 1;
		foreach $engine ( @{ $daemon->{engines} } )
		{
		    next if ($engine->{port} != $2);
		    $engine->{status} = $3;
		    $engine->{connected} = $4;
		    $engine->{channels} = $5;
		    last;
		}
	    }
	}
    }
}


