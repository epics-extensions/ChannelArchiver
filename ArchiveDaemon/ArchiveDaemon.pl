#!/usr/bin/perl -w
#
# ArchiveDaemon is a perl script that reads
# a configuration file which lists what Archive Engines
# should run on the local machine and how.
#
# It will start engines, maybe periodically stop and
# restart them.
# It provides a web server to display the status.
#
# All the ideas in here are adopted from Thomas Birke's
# tcl/tk CAManager/CAbgManager.
#
# kasemir@lanl.gov

use English;
use strict;
use Socket;
use Time::Local;
use File::Basename;
use File::CheckTree;
use File::Path;
use IO::Handle;
use Data::Dumper;
use XML::Simple;
use POSIX 'setsid';
use vars qw($opt_h $opt_p $opt_f);
use Getopt::Std;

# ----------------------------------------------------------------
# Configurables
# ----------------------------------------------------------------

# Setting this to 1 disables(!) caching and might help with debugging.
# $OUTPUT_AUTOFLUSH=1;

# Config file read by ArchiveDaemon
my ($config_file) = "ArchiveDaemon.xml";

# Log file, created in current directory
my ($logfile) = "ArchiveDaemon.log";

# TCP Port of ArchiveDaemon's HTTPD
my ($http_port) = 4610;

# The master index configuration that this tool creates or updates
my ($master_index_config) = "indexconfig.xml";

# The DTD for that file
my ($master_index_dtd) = "indexconfig.dtd";

# The name of the master index to create/update
my ($master_index) = "master_index";

# What ArchiveEngine to use. Just "ArchiveEngine" works if it's in the path.
my ($ArchiveEngine) = "ArchiveEngine";

# Log file of the ArchiveEngine
my ($EngineLog) = "ArchiveEngine.log";

# What ArchiveIndexTool to use. "ArchiveIndexTool" works if it's in the path.
my ($ArchiveIndexTool) = "ArchiveIndexTool -v 1";

# Log file of the Index Tool
my ($ArchiveIndexLog) = "ArchiveIndexTool.log";

# Seconds between "is the engine running?" checks
my ($engine_check_period) = 30;

# Seconds between runs of the ArchiveIndexTool
my ($index_update_period) = 60*60;

# Timeout used for "is there a HTTP client request?"
my ($http_check_timeout) = 1;

# Timeout used when reading a HTTP client or ArchiveEngine
my ($read_timeout) = 5;

# This host. 'localhost' should work unless you have
# more than one network card and a messed up network config.
my ($host) = 'localhost';

# Number of entries in the "Messages" log
my ($message_queue_length) = 20;

# Detach from terminal etc. to run as a background daemon?
my ($daemonization) = 1;

# ----------------------------------------------------------------
# Globals
# ----------------------------------------------------------------

# The configuration of this ArchiveDaemon.
#
# Array where each element is a hash:
# -- read from ArchiveDaemon's config:
# 'desc' => Engine's Description
# 'port' => TCP port of Engine's HTTPD
# 'config' => Configuration file
# 'daily' => "hh:mm" of daily restart
# -- adjusted at runtime
# 'started' => 0 or start time text from running engine.
my (@config);

# Array of index file names (for IndexTool)
my (@indices);

my (@message_queue);

my ($start_time_text);

my ($last_check) = 0;

my ($last_index_update) = 0;

# ----------------------------------------------------------------
# Message Queue
# ----------------------------------------------------------------

sub time_as_text($)
{
    my ($seconds) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	= localtime($seconds);
    return sprintf("%04d/%02d/%02d %02d:%02d:%02d",
		   1900+$year, 1+$mon, $mday, $hour, $min, $sec);
}

sub add_message($)
{
    my ($msg) = @ARG;
    shift @message_queue if ($#message_queue >= $message_queue_length-1);
    push @message_queue, sprintf("%s: <I>%s</I>", time_as_text(time), $msg);
}

# ----------------------------------------------------------------
# Config File
# ----------------------------------------------------------------

# Reads config file into @config
sub read_config($)
{
    my ($file) = @ARG;
    my $parser = XML::Simple->new();
    my $doc = $parser->XMLin($file, ForceArray=>1);
    foreach my $engine ( @{$doc->{engine}} )
    {
	++$#config;
	$config[$#config]{desc} = $engine->{desc}[0];
	$config[$#config]{port} = $engine->{port}[0];
	$config[$#config]{config} = $engine->{config}[0];
	if (defined($engine->{daily}))
	{
	    $config[$#config]{daily} = $engine->{daily}[0];
	}
	$config[$#config]{started} = 0;
    }
}

# ----------------------------------------------------------------
# IndexTool Config File
# ----------------------------------------------------------------

# Reads an IndexTool config according to indexconfig.dtd,
# returning an array of index file names
sub read_indexconfig($)
{
    my ($file) = @ARG;
    my $parser = XML::Simple->new();
    my $doc = $parser->XMLin($file, ForceArray=>1);
    my ($index, @indices);
    foreach $index ( @{$doc->{archive}} )
    {
	push @indices, $index->{index}[0];
    }
    return @indices;
}

# Add a new index file to the array (unless it's already in there)
# Note 1: It needs a REFERENCE to the index array!
# Note 2: Returns "1" if the array was modified
sub add_index($$)
{
    my ($nindex, $indices) = @ARG;
    my ($index);
    foreach $index ( @{$indices} )
    {
	return 0 if ($nindex eq $index);
    }
    # insert new index at head of @indices
    @{$indices} = ( $nindex, @{$indices} );
    return 1;
}

# Write IndexTool config file
sub write_indexconfig($@)
{
    my ($filename, @indices) = @ARG;
    my ($index);
    unless (open(INDEX, ">$filename"))
    {
	add_message("Cannot create $filename");
	return;
    }
    print INDEX "<?xml version=\"1.0\" encoding=\"UTF-8\"" .
	" standalone=\"no\"?>\n";
    print INDEX "<!DOCTYPE indexconfig SYSTEM \"$master_index_dtd\">\n"
	if (length($master_index_dtd) > 0);
    print INDEX "<indexconfig>\n";
    foreach $index ( @indices )
    {
	print INDEX "\t<archive>\n";
	print INDEX "\t\t<index>$index</index>\n";
	print INDEX "\t</archive>\n";
    }
    print INDEX "</indexconfig>\n";
    close(INDEX);
    add_message("Updated $filename");
}

# ----------------------------------------------------------------
# HTTPD Stuff
# ----------------------------------------------------------------

# Stuff prepended/appended to this HTTPD's HTML pages.
sub html_start($$)
{
    my ($client, $refresh) = @ARG;
    my ($web_refresh) = $engine_check_period / 2;
    print $client "<HTML>\n";
    print $client "<META HTTP-EQUIV=\"Refresh\" CONTENT=$web_refresh>\n"
	if ($refresh);
    print $client "<HEAD>\n";
    print $client "<TITLE>Archive Daemon</TITLE>\n";
    print $client "</HEAD>\n";
    print $client "<BODY BGCOLOR=#A7ADC6 LINK=#0000FF " .
	"VLINK=#0000FF ALINK=#0000FF>\n";
    print $client "<FONT FACE=\"Helvetica, Arial\">\n";
    print $client "<BLOCKQUOTE>\n";
}

sub html_stop($)
{
    my ($client) = @ARG;
    print $client "</BLOCKQUOTE>\n";
    print $client "</FONT>\n";
    print $client "<P><HR WIDTH=50% ALIGN=LEFT>\n";
    print $client "<A HREF=\"/\">-Engines-</A>\n";
    print $client "<A HREF=\"/status\">-Status-</A><br>\n";
    print $client scalar localtime;
    print $client "\n";
    print $client "</BODY>\n";
    print $client "</HTML>\n";
}

# $httpd_handle = create_HTTPD($tcp_port)
#
# Call once to create HTTPD.
#
sub create_HTTPD($)
{
    my ($port) = @ARG;
    my ($EOL) = "\015\012";
    socket(HTTP, PF_INET, SOCK_STREAM, getprotobyname('tcp')) 
	or die "Cannot create socket: $!\n";
    setsockopt(HTTP, SOL_SOCKET, SO_REUSEADDR, pack("l", 1))
	or die "setsockopt: $!";
    bind(HTTP, sockaddr_in($port, INADDR_ANY))
        or die "bind: $!";
    listen(HTTP,SOMAXCONN) or die "listen: $!";
    $SIG{CHLD} = 'IGNORE';
    print("Server started on port $port\n");
    return \*HTTP;
}

sub handle_HTTP_main($)
{
    my ($client) = @ARG;
    my ($engine, $message);
    html_start($client, 1);
    print $client "<H1>Archive Daemon</H1>\n";
    print $client "<TABLE BORDER=0 CELLPADDING=5>\n";
    print $client "<TR>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Engine</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Port</FONT></TH>";
    print $client
	"<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Restart</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Status</FONT></TH>";
    print $client "</TR>\n";  
    foreach $engine ( @config )
    {
	print $client "<TR>";
	print $client "<TH BGCOLOR=#FFFFFF>" .
	    "<A HREF=\"http://$host:$engine->{port}\">" .
	    "$engine->{desc}</A></TH>";
	print $client "<TD ALIGN=CENTER>$engine->{port}</TD>";
	if ($engine->{daily})
	{
	    print $client "<TD ALIGN=CENTER>Daily at $engine->{daily}</TD>";
	}
	else
	{
	    print $client "<TD ALIGN=CENTER>-</TD>";
	}	
	if ($engine->{started})
	{
	    print $client "<TD ALIGN=CENTER>Started $engine->{started}</TD>";
	}
	else
	{
	    print $client "<TD ALIGN=CENTER><FONT color=#FF0000>" . 
		"Not Running</FONT></TD>";
	}
       
	print $client "</TR>\n";
    }
    print $client "</TABLE>\n";
    print $client "<H2>Messages</H2>\n";
    foreach $message ( reverse @message_queue )
    {
	print $client "$message<BR>\n";
    }
    html_stop($client);
}

sub handle_HTTP_status($)
{
    my ($client) = @ARG;
    my ($engine, $total, $running);
    html_start($client, 1);
    print $client "<H1>Archive Daemon</H1>\n";
    $total = $#config + 1;
    $running = 0;
    foreach $engine ( @config )
    {
	++$running if ($engine->{started});
    }
    print $client "<FONT COLOR=#FF0000>" if ($running != $total);
    print $client "$running of $total engines are running\n";
    print $client "</FONT>" if ($running != $total);
    html_stop($client);
}

sub handle_HTTP_info($)
{
    my ($client) = @ARG;
    html_start($client, 1);
    print $client "<H1>Archive Daemon</H1>\n";
    print $client "Config File: $config_file<p>\n";
    print $client "Daemon start time: $start_time_text<p>\n";
    print $client "Last Check: ", time_as_text($last_check), "<p>\n";
    print $client "Index Update: ", time_as_text($last_index_update), "<p>\n";
    html_stop($client);
}

# Used by check_HTTPD to dispatch requests
sub handle_HTTP_request($$)
{
    my ($doc, $client) = @ARG;
    my ($URL);
    if ($doc =~ m/GET (.+) HTTP/)
    {
	$URL = $1;
	if ($URL eq '/')
	{
	    handle_HTTP_main($client);
	    return 1;
	}
	elsif ($URL eq '/status')
	{
	    handle_HTTP_status($client);
	    return 1;
	}
	elsif ($URL eq '/info')
	{
	    handle_HTTP_info($client);
	    return 1;
	}
	elsif ($URL eq '/stop')
	{
	    html_start($client, 0);
	    print $client "Quitting.\n";
	    html_stop($client);
	    return 0;
	}
	else
	{
	    print $client "You requested: '<I>" . $URL . "</I>'\n";
	    return 1;
	}
    }
    print $client "<B>Error</B>\n";
    return 1;
}

# check_HTTPD($httpd_handle)
#
# Call periodically to check for HTTP clients
#
sub check_HTTPD($)
{
    my ($sock) = @ARG;
    my ($c_addr, $smask, ,$smask_in, $num);
    $smask_in = '';
    vec($smask_in, fileno($sock), 1) = 1;
    $num = select($smask=$smask_in, undef, undef, $http_check_timeout);
    return if ($num <= 0);
    if ($c_addr = accept(CLIENT, $sock))
    {
	#my($c_port,$c_ip) = sockaddr_in($c_addr);
	#print "HTTP Client ", inet_ntoa($c_ip), ":", $c_port, "\n";
	# Read the client's request
	my ($mask, ,$mask_in, $doc, $line);
	$doc = '';
	$mask_in = '';
	vec($mask_in, fileno(CLIENT), 1) = 1;
	$num = select($mask=$mask_in, undef, undef, $read_timeout);
	while ($num > 0)
	{
	    $num = sysread CLIENT, $line, 5000;
	    last if (not defined($num));
	    last if ($num == 0);
	    $line =~ s[\r][]g;
	    $doc = $doc . $line;
	    #print("Line: ($num) '$line'\n");
	    #print("Doc: '$doc'\n");
	    if ($doc =~ m/\n\n/s)
	    {
		#print "Found end of request\n";
		last;
	    }
		$num = select($mask=$mask_in, undef, undef, $read_timeout);
	}
	# Respond
	print CLIENT "HTTP/1.1 200 OK\n";
	print CLIENT "Server: Apache/2.0.40 (Red Hat Linux)\n";
	print CLIENT "Connection: close\n";
	print CLIENT "Content-Type: text/html\n";
	print CLIENT "\n"; 
	my ($continue) = handle_HTTP_request($doc, \*CLIENT);
	close CLIENT;
	if (not $continue)
	{
	    print("Quitting.\n");
	    exit(0);
	}
    }
}    

# ----------------------------------------------------------------
# Engine Start/Stop Stuff
# ----------------------------------------------------------------

# Return what the given engine should use for an index name,
# including the relative path with YYYY/MM_DD etc. if applicable.
sub make_indexname($$)
{
    my ($now, $engine) = @ARG;
    if (defined($engine->{daily}))
    {
	my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	    = localtime($now);
	return sprintf("%04d/%02d_%02d/index", 1900+$year, 1+$mon, $mday);
    }
    else
    {
	return "index";
    }
}

# Attempt to start one engine
# Returns 1 if the master index config. was updated
sub start_engine($$)
{
	my ($now, $engine) = @ARG;
	my ($null) = "/dev/null";
	my ($dir) = dirname($engine->{config});
	my ($cfg) = basename($engine->{config});
	my ($index) = make_indexname($now, $engine);
	add_message(
	       "Starting Engine '$engine->{desc}': $host:$engine->{port}\n");
	my ($path) = dirname("$dir/$index");
	if (not -d $path)
	{
	    add_message("Creating dir '$path'\n");
	    mkpath($path);
	}
	my ($cmd) = "cd \"$dir\";" .
	    "$ArchiveEngine -d \"$engine->{desc}\" -l $EngineLog " .
	    "-p $engine->{port} $cfg $index >$null 2>&1 &";
	print(time_as_text(time), ": Command: '$cmd'\n");
	system($cmd);
	if (add_index("$dir/$index", \@indices))
	{
	    write_indexconfig($master_index_config, @indices);
	    return 1;
	}
	return 0;
}

sub run_indextool()
{
    my ($dir, $cfg, $cmd);
    $dir = dirname($master_index_config);
    $cfg = basename($master_index_config);
    $cmd = "cd $dir;$ArchiveIndexTool $cfg master_index " .
	">$ArchiveIndexLog 2>&1 &";
    print(time_as_text(time), ": Command: '$cmd'\n");
    add_message("Running Index Tool");
    system($cmd);
    $last_index_update = time;
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

# Stop ArchiveEngine on host/port
sub stop_engine($$)
{
    my ($host, $port) = @ARG;
    my (@doc, $line);
    add_message("Stopping engine $host:$port");
    @doc = read_URL($host, $port, "/stop");
    foreach $line ( @doc )
    {
	return 1 if ($line =~ m'Engine will quit');
    }
    print(time_as_text(time),
	  ": Engine $host:$port won't quit.\nResponse : @doc\n");
    return 0;
}

# Test if ArchiveEngine runs on host/port, returning description & start time
sub check_engine($$)
{
    my ($host, $port) = @ARG;
    my ($line, $desc, $started);
    $desc = '';
    $started = '';
    foreach $line ( read_URL($host, $port, "/") )
    {
	if ($line =~ m'Description</TH>.*>([^>]+)</TD>')
	{
	    $desc = $1;
	}
	if ($line =~ m'Started</TH>[^0-9]+([0123456789/:. ]+)')
	{
	    $started = $1;
	}
    }
    return ($desc, $started);
}

# Check if now's the time to restart a given engine
sub check_restart($$)
{
    my ($now, $engine) = @ARG;
    my ($n_sec,$n_min,$n_hour,$n_mday,$n_mon,$n_year,$wday,$yday,$isdst);
    my ($e_mon, $e_mday, $e_year, $e_hour, $e_min, $e_sec, $nano);
    my ($engine_secs, $restart_secs);
    
    ($n_sec,$n_min,$n_hour,$n_mday,$n_mon,$n_year,$wday,$yday,$isdst)
	= localtime($now);
    ($e_mon, $e_mday, $e_year, $e_hour, $e_min, $e_sec, $nano)
	= split '[/ :.]', $engine->{started};
    $engine_secs
	= timelocal($e_sec,$e_min,$e_hour,$e_mday,$e_mon-1,$e_year-1900);
    if (defined($engine->{daily}))
    {
	my ($hour, $minute) = split ':', $engine->{daily};
	$restart_secs = timelocal(0,$minute,$hour,$n_mday,$n_mon,$n_year);
	# Restart if we're past the restart time and the engine's too old:
	if ($now > $restart_secs and $engine_secs < $restart_secs)
	{
	    stop_engine($host, $engine->{port});
	}
    }
}

# For all engines marked in the config as running, check when to restart
sub check_restarts($)
{
    my ($now) = @ARG;
    my ($engine);
    foreach $engine ( @config )
    {
	next unless ($engine->{started});
	check_restart($now, $engine);
    }
}

# Query all engines in config, check if they're running
sub check_engines($)
{
    my ($now) = @ARG;
    my ($engine, $desc, $started);
    foreach $engine ( @config )
    {
	($desc, $started) = check_engine($host, $engine->{port});
	if (length($started) > 0)
	{
	    $engine->{started} = $started;
	}
	else
	{
	    $engine->{started} = 0;
	}
    }
}

# Attempt to start all engines in config that are not marked as 'running'
sub start_engines($)
{
    my ($now) = @ARG;
    my ($engine, $desc, $started, $index_changed);
    $index_changed = 0;
    foreach $engine ( @config )
    {
	next if ($engine->{started});
	$index_changed += start_engine($now, $engine);
    }
    if ($index_changed > 0)
    {
	run_indextool();
    }
}

# ----------------------------------------------------------------
# Main
# ----------------------------------------------------------------
sub usage()
{
    print("USAGE: ArchiveDaemon [options]\n");
    print("\n");
    print("Options:\n");
    print("\t-p <port>: TCP port number for HTTPD\n");
    print("\t-f file  : use file instead of $config_file\n");
    print("\n");
    print("This tool automatically starts, monitors and restarts\n");
    print("ArchiveEngines based on $config_file.\n");
    exit(-1);
}
if (!getopts('hp:f:')  ||  $#ARGV != -1  ||  $opt_h)
{
    usage();
}
# Allow command line options to override various defaults
$http_port = $opt_p if ($opt_p);
$config_file = $opt_f if ($opt_f);
add_message("Started");
read_config($config_file);
if (length($master_index_config) > 0)
{
    @indices = read_indexconfig($master_index_config);
}  
print("Read $config_file, will disassociate from terminal\n");
print("and from now on only respond via\n");
print("          http://localhost:$http_port\n");
print("You can also monitor the log file:\n");
print("          $logfile\n");
# Daemonization, see "perldoc perlipc"
if ($daemonization)
{
    open STDIN, "/dev/null" or die "Cannot disassociate STDIN\n";
    open STDOUT, ">$logfile" or die "Cannot create $logfile\n";
    defined(my $pid = fork) or die "Can't fork: $!";
    exit if $pid;
    setsid                  or die "Can't start a new session: $!";
    open STDERR, '>&STDOUT' or die "Can't dup stdout: $!";
}
$start_time_text = time_as_text(time);
my ($httpd) = create_HTTPD($http_port);
my ($now);
$last_index_update = 0;
while (1)
{
    $now = time;
    if (($now - $last_check) > $engine_check_period)
    {
	check_restarts($now);
	check_engines($now);
	start_engines($now);
	$last_check = time;
    }
    if (($now - $last_index_update) > $index_update_period)
    {
	run_indextool();
    }
    check_HTTPD($httpd);
}
