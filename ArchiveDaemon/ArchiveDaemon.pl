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
use Sys::Hostname;
use Time::Local qw(timelocal timelocal_nocheck);
use File::Basename;
use File::CheckTree;
use File::Path;
use IO::Handle;
use Data::Dumper;
use XML::Simple;
use POSIX 'setsid';
use vars qw($opt_h $opt_p $opt_f $opt_i $opt_u $opt_d);
use Getopt::Std;

# ----------------------------------------------------------------
# Configurables
# ----------------------------------------------------------------

# Compare: manual/changes.tex
my ($version) = "2.3.0";

# Setting this to 1 disables(!) caching and might help with debugging.
# Default: leave it commented-out.
# $OUTPUT_AUTOFLUSH=1;

# Config file read by ArchiveDaemon. If left empty,
# one has to use the -f option.
my ($config_file) = "";

# Log file, created in current directory.
# No good reason to change this one.
my ($logfile) = "ArchiveDaemon.log";

# Name of this host. Used to connect ArchiveEngines on this host,
# also part of the links on the daemon's web pages.
# Possible options:
# 1) Fixed string 'localhost'.
#    This will get you started, won't require DNS or any
#    other setup, and will always work for engines and web
#    clients on the local machine.
#    Won't work with web clients on other machines,
#    since they'll then follow links to 'localhost'.
# 2) Fixed string 'name.of.your.host'.
#    Will also always work plus allow web clients from
#    other machines on the network, but you have to assert
#    that your fixed string is correct.
# 3) hostname() function.
#    In theory, this is the best method.
#    In practice, it might fail when you have more than one
#    network card or no DNS.
#my ($localhost) = 'localhost';
#my ($localhost) = 'ics-srv-archive1';
my ($localhost) = hostname();

# Default TCP Port of ArchiveDaemon's HTTPD
my ($http_port) = 4610;

# The default DTD for the index config file
# (unless we could parse it from an existing one)
my ($master_index_dtd) = "indexconfig.dtd";

# The master index configuration that this tool creates or updates
# Probably a bad idea to change this one.
my ($index_config) = "indexconfig.xml";

# The reduced master index configuration that this tool creates,
# omitting sub-archives whose indices are much older than
# the master_index and thus needn't be checked over and over again
# (see $index_omit_age).
# Probably a bad idea to change this one.
my ($index_update_config) = "indexupdate.xml";

# If a sub-archive's index is this number of days older
# than the master index, we omit it in the update process:
# It's still listed in the complete $index_config,
# but commented out in $index_update_config.
my ($index_omit_age) = 1.0;

# The name of the master index to create/update.
# Probably a bad idea to change this one.
my ($master_index) = "master_index";

# What ArchiveEngine to use. Just "ArchiveEngine" works if it's in the path.
my ($ArchiveEngine) = "ArchiveEngine";

# What ArchiveIndexTool to use. "ArchiveIndexTool" works if it's in the path.
my ($ArchiveIndexTool) = "ArchiveIndexTool -v 1";

# Log file of the Index Tool. No good reason to change this one.
my ($ArchiveIndexLog) = "ArchiveIndexTool.log";

# Seconds between "is the engine running?" checks.
# We _hope_ that the engine is running all the time
# and only want to restart e.g. once a day,
# so checking every 30 seconds is more than enough
# yet gives us a fuzzy feeling that the daemon's web
# page shows the current state of things.
my ($engine_check_secper) = 30;

# Minutes between runs of the ArchiveIndexTool.
# 60 = 1 hour ?
# This means data in the master index is up to 1 hour old.
# Can be a smaller number. You need to check how long
# the index tool runs. You don't want it running all the time.
my ($index_update_minper) = 60;

# Most of the time, the ArchiveIndexTool is run with
# the $index_update_config.
# But about once a day it's a good idea to
# run it will the full $index_config.
my ($full_index_minper) = 24*60;

# Timeout used for "is there a HTTP client request?"
# 1 second gives good response yet daemon uses hardly any CPU.
my ($http_check_timeout) = 1;

# Timeout used when reading a HTTP client or ArchiveEngine.
# 10 seconds should be reasonable. Trying 30sec at SNS because
# the archive server is often very busy.
my ($read_timeout) = 30;

# Number of entries in the "Messages" log.
my ($message_queue_length) = 25;

# Detach from terminal etc. to run as a background daemon?
# 1 should always be OK unless you're doing strange debugging.
my ($daemonization) = 1;

# ----------------------------------------------------------------
# Globals
# ----------------------------------------------------------------

# The runtime configuration of this ArchiveDaemon.
#
# Array[] where each element is a hash:
# -- read from ArchiveDaemon's config:
# 'desc'       => Engine's Description
# 'port'       => TCP port of Engine's HTTPD
# 'config'     => Configuration file
# 'daily'      => undef or "hh:mm" for daily restart
# 'day'        => undef or number of weekday for weekly restart
# 'hourly'     => undef or (double) hours between restarts
# -- adjusted at runtime
# 'next_start' => next date/time when engine should start
# 'next_stop'  => next .... stop
# 'started'    => '' or start time info of the running engine.
# 'channels'   => # of channels (only valid if started)
# 'connected'  => # of _connected_ channels (only valid if started)
# 'disabled'   => is there a file 'DISABLED.txt' ?
# 'lockfile'   => is there a file 'archive_active.lck' ?
my (@config);

# Array of index file names (for IndexTool)
my (@indices);

my (@message_queue);
my ($start_time_text);
my ($last_check) = 0;
my ($last_index_update) = 0;
my ($last_full_index) = 0;

# TODO: This looks bad.
my (%weekdays, @weekdays);
$weekdays{Su} = 0;
$weekdays{Mo} = 1;
$weekdays{Tu} = 2;
$weekdays{We} = 3;
$weekdays{Th} = 4;
$weekdays{Fr} = 5;
$weekdays{Sa} = 6;
$weekdays[0] = "Sunday";
$weekdays[1] = "Monday";
$weekdays[2] = "Tuesday";
$weekdays[3] = "Wednesday";
$weekdays[4] = "Thursday";
$weekdays[5] = "Friday";
$weekdays[6] = "Saturday";

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

sub time_as_short_text($)
{
    my ($seconds) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	= localtime($seconds);
    return sprintf("%04d_%02d_%02d-%02d_%02d_%02d",
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

sub get_DTD($)
{
    my ($file) = @ARG;
    my ($dtd) = "";
    if (open F, $file)
    {
	while (<F>)
	{
	    if ($_ =~ m'<!DOCTYPE[^<>]+SYSTEM "(.+)".*>')
	    {
		$dtd = $1;
		last;
	    }
	}
	close F;
    }
    return $dtd;
}

# Reads config file into @config
sub read_config($)
{
    my ($file) = @ARG;
    my ($parser, $doc, $engine);
    $parser = XML::Simple->new();
    $doc = $parser->XMLin($file, ForceArray=>1);
    foreach $engine ( @{$doc->{engine}} )
    {
	++$#config;
	$config[$#config]{desc} = $engine->{desc}[0];
	$config[$#config]{port} = $engine->{port}[0];
	$config[$#config]{config} = $engine->{config}[0];
	if (defined($engine->{weekly}))
	{
	    my ($daytxt, $daily) = split(/ /, $engine->{weekly}[0]);
	    my ($daychars) = substr($daytxt, 0, 2);
	    die ("Engine '$config[$#config]{desc}': " .
		 "Unknown day name '$daytxt' " .
		 "in weekly config $engine->{weekly}[0]")
		unless defined($weekdays{$daychars});	    
	    $config[$#config]{day} = $weekdays{$daychars};
	    $config[$#config]{daily} = $daily;
	}
	elsif (defined($engine->{daily}))
	{
	    $config[$#config]{daily} = $engine->{daily}[0];
	}
	elsif (defined($engine->{hourly}))
	{
	    $config[$#config]{hourly} = $engine->{hourly}[0];
	}
	elsif (defined($engine->{timed}))
	{
	    $config[$#config]{timed} = $engine->{timed}[0];
	}
	$config[$#config]{started} = '';
    }
}

# Update the start/stop times based on
# current state of the engine
# and the daily/hourly/... configuration.
sub update_schedule($)
{
    my ($now) = @ARG;
    my ($engine, $engine_start_secs);
    my ($n_sec,$n_min,$n_hour,$n_mday,$n_mon,$n_year,$n_wday,$x);
    my ($e_mon, $e_mday, $e_year, $e_hour, $e_min, $e_sec);
    my ($hour, $minute, $hours, $minutes);

    print " ------ update_schedule  : ", time_as_text($now), "\n";
    ($n_sec,$n_min,$n_hour,$n_mday,$n_mon,$n_year,$n_wday,$x,$x) =
	localtime($now);
    foreach $engine ( @config )
    {
	print " Engine $engine->{desc} ";
	# Get engine_start_secs
	if (length($engine->{started}) > 0)
	{
	    ($e_mon, $e_mday, $e_year, $e_hour, $e_min, $e_sec)
		= split '[/ :.]', $engine->{started};
	    $engine_start_secs
		= timelocal($e_sec,$e_min,$e_hour,$e_mday,
			    $e_mon-1,$e_year-1900);
	    print "  Started: $engine->{started} = ",
	          time_as_text($engine_start_secs), "\n";
	}
	else
	{
	    print "  Not running\n";
	    $engine_start_secs = 0;
	}
	# Determine next_start, next_stop
	if (defined($engine->{daily}))
	{   
	    if ($engine_start_secs <= 0)
	    {   # Not Running: Start as soon as possible
		$engine->{next_start} = $now;
		$engine->{next_stop}  = 0;
	    }
	    else
	    {   # Running: Determine stop time
		($hour, $minute) = split ':', $engine->{daily};
		$engine->{next_start} = 0;
		if (defined($engine->{day}))
		{   # Target: $engine->{day}; Today's daynum: $n_wday
		    my ($days_to_go) = $engine->{day}-$n_wday;
		    $days_to_go += 7 if ($days_to_go < 0);
		    $engine->{next_stop} =
			timelocal_nocheck(0, $minute,$hour,
					  $n_mday+$days_to_go,$n_mon, $n_year);
		    # Already happened today, so we'll try again next week?
		    $engine->{next_stop} += 24*60*60*7
			if ($engine_start_secs > $engine->{next_stop});
		}
		else
		{
		    $engine->{next_stop} = timelocal(0,$minute,$hour,
						     $n_mday,$n_mon,$n_year);
		    # Today or already into the next day?
		    $engine->{next_stop} += 24*60*60
			if ($engine_start_secs > $engine->{next_stop});
		}
	    }
	}
	elsif (defined($engine->{hourly}))
	{
	    if ($engine_start_secs <= 0)
	    {   # Not Running: Start as soon as possible
		$engine->{next_start} = $now;
		$engine->{next_stop}  = 0;
	    }
	    else
	    {   # Running: Determine stop time (relative to engine)
		my ($rounding) = int($engine->{hourly} * 60*60);
		$engine->{next_start} = 0;
		$engine->{next_stop} =
		    (int($engine_start_secs/$rounding)+1) * $rounding;
	    }
	}
	elsif (defined($engine->{timed}))
	{   
	    ($hour, $minute, $hours, $minutes) = split '[:/]',$engine->{timed};
	    # Calc start/end
	    my ($start) = timelocal(0,$minute,$hour,$n_mday,$n_mon,$n_year);
	    my ($stop) = $start + ($hours*60 + $minutes) * 60;
	    if ($engine_start_secs > 0)
	    {   # Running: Observe stop time
		$engine->{next_start} = 0;
		$engine->{next_stop} = $stop;
	    }
	    elsif ($start <= $now  and  $now <= $stop)
	    {   # Not running but should
		$engine->{next_start} = $start;
		$engine->{next_stop} = 0;
	    }
	    else
	    {   # Not running and shouldn't
		$engine->{next_start} = 0;
		$engine->{next_stop} = 0;
	    }
	}
	else
	{   
	    if ($engine_start_secs <= 0)
	    {   # Not Running: Start as soon as possible
		$engine->{next_start} = $now;
		$engine->{next_stop}  = 0;
	    }
	    else
	    {   # Running: keep going w/o end
		$engine->{next_start} = $engine->{next_stop} = 0;
	    }
	}
	print "   Next start: ", time_as_text($engine->{next_start}), "\n"
	    if ($engine->{next_start} > 0);
	print "   Next stop : ", time_as_text($engine->{next_stop}),  "\n"
	    if ($engine->{next_stop} > 0);
    }
}

# ----------------------------------------------------------------
# IndexTool Config File
# ----------------------------------------------------------------

# Reads an IndexTool config according to indexconfig.dtd
sub read_indexconfig($)
{
    my ($file) = @ARG;
    if (-f $file)
    {
	my $parser = XML::Simple->new();
	my $doc = $parser->XMLin($file, ForceArray=>1);
	foreach my $index ( @{$doc->{archive}} )
	{
	    push @indices, $index->{index}[0];
	}
    }
}

# Add a new index file to the array (unless it's already in there)
# Returns "1" if the array was modified
sub add_index($)
{
    my ($nindex) = @ARG;
    my ($index);
    foreach $index ( @indices )
    {
	return 0 if ($nindex eq $index);
    }
    # insert new index at head of @indices
    @indices = ( $nindex, @indices );
    return 1;
}

# Write IndexTool config file
# For a _full_ update, we include
# every sub-index in the master index config.
# Otherwise, sub-indices that are older than
# the master index (by $index_omit_age in days)
# are omitted, assuming they're already in the master.
sub write_indexconfig($)
{
    my ($full) = @ARG;
    my ($filename, $index, $skip, $index_age, $diff, $why);
    $filename = $index_update_config;
    $filename = $index_config if ($full);
    $skip = 0;
    if (-f $master_index)
    {
	$index_age = -M $master_index;
    }
    else
    {   # Need full update if master doesn't exist, yet.
	$full = 1;
    }
    unless (open(INDEX, ">$filename"))
    {
	add_message("Cannot create $filename");
	return;
    }
    print INDEX "<?xml version=\"1.0\" encoding=\"UTF-8\"" .
	" standalone=\"no\"?>\n";
    print INDEX "<!--        Written by the ArchiveDaemon        -->\n";
    print INDEX "<!-- Do not edit while ArchiveDaemon is running -->\n";
    print INDEX "<!DOCTYPE indexconfig SYSTEM \"$master_index_dtd\">\n"
	if (length($master_index_dtd) > 0);
    print INDEX "<indexconfig>\n";
    foreach $index ( @indices )
    {
	if (not $full)
	{
            if (-f $index)
            {
		$diff = (-M $index) - $index_age;
		$skip = $diff > $index_omit_age;
		$why  = "$diff days older than $master_index";
            }
	    else
	    {
		$skip = 1;
		$why  = "Doesn't exist (yet?)";
	    }
	}
	print INDEX "\t<!-- skipped: $why\n" if ($skip);
	print INDEX "\t<archive>\n";
	print INDEX "\t\t<index>$index</index>\n";
	print INDEX "\t</archive>\n";
	print INDEX "\t-->\n" if ($skip);
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
    my ($web_refresh) = $engine_check_secper / 2;
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
    print $client "<A HREF=\"/info\">-Info-</A>  \n";
    print $client time_as_text(time);
    print $client "<br>\n";
    print $client "<I>Note that this page updates only ";
    print $client "every $engine_check_secper seconds!</I>\n";
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
    print("Server started on port $port\n");
    return \*HTTP;
}

sub handle_HTTP_main($)
{
    my ($client) = @ARG;
    my ($engine, $message, $channels, $connected);
    html_start($client, 1);
    print $client "<H1>Archive Daemon</H1>\n";
    print $client "<TABLE BORDER=0 CELLPADDING=5>\n";
    print $client "<TR>";
    # TABLE:   Engine  |  Port  |  Started  |  Status  |  Restart
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Engine</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Port</FONT></TH>";
    print $client
	"<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Started</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Status</FONT></TH>";
    print $client
	"<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Restart</FONT></TH>";
    print $client "<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>Action</FONT></TH>";
    print $client "</TR>\n";
    foreach $engine ( @config )
    {
	print $client "<TR>";
	print $client "<TH BGCOLOR=#FFFFFF>" .
	    "<A HREF=\"http://$localhost:$engine->{port}\">" .
	    "$engine->{desc}</A></TH>";
	print $client "<TD ALIGN=CENTER>$engine->{port}</TD>";
	if ($engine->{started})
	{
	    print $client "<TD ALIGN=CENTER>$engine->{started}</TD>";
	    $connected = $engine->{connected};
	    $channels  = $engine->{channels};
	    print $client "<TD ALIGN=CENTER>";
	    print $client "<FONT COLOR=#FF0000>" if ($channels != $connected);
	    print $client "$connected/$channels channels connected";
	    print $client "</FONT>" if ($channels != $connected);
	    if ($engine->{disabled})
	    {
		print $client "<br><FONT COLOR=#FF0000>";
		print $client "Found DISABLED file.";
		print $client "</FONT>";
	    }
	    print $client "</TD>";
	}
	else
	{
	    print $client "<TD></TD>";
	    if ($engine->{lockfile})
	    {
		print $client "<TD ALIGN=CENTER><FONT color=#FF0000>" .
		    "Unknown. Found lock file.</FONT><br>";
                print $client "(Check again, see if issue persists)</TD>";
	    }
	    elsif ($engine->{disabled})
	    {
		print $client "<TD ALIGN=CENTER><FONT color=#FFFF00>" .
		    "Disabled.</FONT></TD>";
	    }
	    else
	    {
		print $client "<TD ALIGN=CENTER><FONT color=#FF0000>" .
		    "Not running.</FONT>";
	    }
	}
	print $client "<TD ALIGN=CENTER>";
	if ($engine->{daily})
	{
	    if (defined($engine->{day}))
	    {
		print $client "Every $weekdays[$engine->{day}] " .
		    "at $engine->{daily}.";
	    }
	    else
	    {
		print $client "Daily at $engine->{daily}.";
	    }
	}
	elsif ($engine->{hourly})
	{
	    print $client "Every $engine->{hourly} h.";
	}
	elsif ($engine->{timed})
	{
	    print $client "Start/Duration: $engine->{timed}";
	}
	else
	{
	    print $client "Continuously running.";
	}
	if ($engine->{next_start} > 0)
	{
	    print $client
		"<br>Next start ", time_as_text($engine->{next_start});
	}
	if ($engine->{next_stop} > 0)
	{
	    print $client
		"<br>Next stop ", time_as_text($engine->{next_stop});
	}
	print $client "</TD>";
	if ($engine->{disabled})
	{
	    print $client
		"<TD><A HREF=\"enable/$engine->{port}\">Enable</A></TD>";
	}
	else
	{
	    print $client
		"<TD><A HREF=\"disable/$engine->{port}\">Disable</A></TD>";
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
    my ($engine, $total, $disabled, $running, $channels, $connected);
    html_start($client, 1);
    print $client "<H1>Archive Daemon Status</H1>\n";
    print $client "# Description|Port|Running|Connected|Channels<BR>\n";  
    $total = $#config + 1;
    $disabled = $running = $channels = $connected = 0;
    foreach $engine ( @config )
    {
	print $client "ENGINE $engine->{desc}|";
	print $client "$engine->{port}|";
	if ($engine->{disabled})
	{
	    print $client "disabled|0|0<br>\n";
	    ++$disabled;
	}
	elsif ($engine->{started})
	{
	    ++$running;
	    $channels += $engine->{channels};
	    $connected += $engine->{connected};
	    print $client "running|$engine->{connected}|$engine->{channels}<br>\n";
	}
	else
	{
	    print $client "down|0|0<br>\n";
	}
    }
    print $client "# engines|disabled|running|connected|channels<BR>\n";  
    print $client "#TOTAL $total|$disabled|$running|$connected|$channels<BR>\n";  
    html_stop($client);
}

sub handle_HTTP_info($)
{
    my ($client) = @ARG;
    html_start($client, 1);
    print $client "<H1>Archive Daemon Info</H1>\n";
    print $client "<TABLE BORDER=0 CELLPADDING=5>\n";

    print $client "<TR><TD><B>Version:</B></TD><TD>$version</TD></TR>\n";
    print $client "<TR><TD><B>Config File:</B></TD><TD>$config_file</TD></TR>\n";
    print $client "<TR><TD><B>Daemon start time:</B></TD><TD> $start_time_text</TD></TR>\n";
    print $client "<TR><TD><B>Check Period:</B></TD><TD> $engine_check_secper secs</TD></TR>\n";
    print $client "<TR><TD><B>Last Check:</B></TD><TD> ", time_as_text($last_check), "</TD></TR>\n";
    print $client "<TR><TD><B>Index DTD:</B></TD><TD> $master_index_dtd</TD></TR>\n";
    print $client "<TR><TD><B>Full Index Period:</B></TD><TD> $full_index_minper minutes</TD></TR>\n";
    print $client "<TR><TD><B>Last:</B></TD><TD> ", time_as_text($last_full_index), "</TD></TR>\n";
    print $client "<TR><TD><B>Index Update Period:</B></TD><TD> $index_update_minper minutes</TD></TR>\n";
    print $client "<TR><TD><B>Last:</B></TD><TD> ", time_as_text($last_index_update), "</TD></TR>\n";
    print $client "</TABLE>\n";
    html_stop($client);
}

sub handle_HTTP_postal($)
{
    my ($client) = @ARG;
    my ($engine);
    html_start($client, 0);
    print $client "<H1>Postal Archive Daemon</H1>\n";
    foreach $engine ( @config )
    {
	next unless ($engine->{started});
	print $client "Stopping " .
	    "<A HREF=\"http://$localhost:$engine->{port}\">" .
	    "$engine->{desc}</A> on port $engine->{port}<br>\n";
	stop_engine($localhost, $engine->{port});
    }
    print $client "Quitting<br>\n";
    html_stop($client);
}

sub handle_HTTP_disable($$)
{
    my ($client, $engine_port) = @ARG;
    my ($engine, $dir);
    html_start($client, 0);
    print $client "<H1>Disabling Engines</H1>\n";
    foreach $engine ( @config )
    {
	if ($engine->{port} == $engine_port)
	{
	    print $client "<H2>Engine '$engine->{desc}'</H2>\n";
	    $dir = dirname($engine->{config});
	    if (open(DISABLED, ">$dir/DISABLED.txt"))
	    {
		print DISABLED "Disabled by ArchiveDaemon, ";
		print DISABLED time_as_text(time);
		print DISABLED "\n";
		close(DISABLED);
		print $client "Created 'DISABLED.txt' in '$dir'.<p>\n";
		stop_engine($localhost, $engine->{port});
		$engine->{disabled} = 1;
		print $client "Stopped via port $engine->{port}.\n";
		print "Disabled '$engine->{desc}, port $engine->{port}.\n";
	    }
	    else
	    {
		print $client "ERROR: Cannot create 'DISABLED.txt' in '$dir'.";
	    }
	    last;
	}
    }
    html_stop($client);
}

sub handle_HTTP_enable($$)
{
    my ($client, $engine_port) = @ARG;
    my ($engine, $dir);
    html_start($client, 0);
    print $client "<H1>Enabling Engines</H1>\n";
    foreach $engine ( @config )
    {
	if ($engine->{port} == $engine_port)
	{
	    print $client "<H2>Engine '$engine->{desc}'</H2>\n";
	    $dir = dirname($engine->{config});
	    if (unlink("$dir/DISABLED.txt") == 1)
	    {
		$engine->{disabled} = 0;
		$last_check = 0; # Force immediate check of engines & maybe restart
		print $client "Removed 'DISABLED.txt' from '$dir'.<p>\n";
		print $client "Daemon will eventually start the engine ";
		print $client "at the next check interval.\n";
		print "Enabled '$engine->{desc}, port $engine->{port}.\n";
	    }
	    else
	    {
		print $client "ERROR: Cannot find 'DISABLED.txt' in '$dir'.";
	    }
	    last;
	}
    }
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
	elsif ($URL eq '/postal')
	{
	    handle_HTTP_postal($client);
	    return 0;
	}
	elsif ($URL =~ m'/disable/([0-9]+)')
	{
	    handle_HTTP_disable($client, $1);
	    return 1;
	}
	elsif ($URL =~ m'/enable/([0-9]+)')
	{
	    handle_HTTP_enable($client, $1);
	    return 1;
	}
	else
	{
	    print $client "<H1>Unknown URL</H1>\n";
	    print $client "You requested: '<I>" . $URL . "</I>',\n";
	    print $client "but this URL is not handled by the ArchiveDaemon.\n";
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
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	= localtime($now);
    if (defined($engine->{hourly}))
    {
	return sprintf("%04d/%02d_%02d_%02dh/index",
		       1900+$year, 1+$mon, $mday, $hour);
    }
    if (defined($engine->{daily})  or  defined($engine->{timed}))
    {
	return sprintf("%04d/%02d_%02d/index", 1900+$year, 1+$mon, $mday);
    }
    return "index";
}

sub run_indextool($)
{
    my ($now) = @ARG;
    my ($dir, $cfg, $cmd, $config);
    $config = $index_update_config;
    if (($now - $last_full_index) > ($full_index_minper*60))
    {
	$config = $index_config;
	$last_full_index = $now;
    }
    $dir = dirname($config);
    $cfg = basename($config);
    $cmd = "cd $dir;$ArchiveIndexTool $cfg $master_index " .
	">$ArchiveIndexLog 2>&1 &";
    print(time_as_text(time), ": Command: '$cmd'\n");
    add_message("Running Index Tool w/ $config");
    system($cmd);
    $last_index_update = $now;
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

# Test if ArchiveEngine runs on host/port,
# returning (description, start time, # channels, # connected channels)
sub check_engine($$)
{
    my ($host, $port) = @ARG;
    my ($line, $desc, $started, $channels, $connected);
    $desc = '';
    $started = '';
    $channels = $connected = 0;
    foreach $line ( read_URL($host, $port, "/") )
    {
	if ($line =~ m'Description</TH>.*>([^>]+)</TD>')
	{
	    $desc = $1;
	}
	elsif ($line =~ m'Started</TH>.+(\d{2}/\d{2}/\d{4} \d{2}:\d{2}:\d{2})')
	{   #     Compare: Started</TH>... 03/17/2004 18:01:01.208511000
	    $started = $1;
	}
	elsif ($line =~ m'Channels</TH>[^0-9]+([0123456789]+)')
	{
	    $channels = $1;
	}
	elsif ($line =~ m'Connected</TH>.+>([0123456789]+)<')
	{
	    $connected = $1;
	}
    }
    return ($desc, $started, $channels, $connected);
}

# Query all engines in config, check if they're running
sub check_engines($)
{
    my ($now) = @ARG;
    my ($engine, $desc, $started, $channels, $connected);
    print(" -------------------- check_engines\n") if ($opt_d);
    foreach $engine ( @config )
    {
	my ($dir) = dirname($engine->{config});
	$engine->{disabled} = (-f "$dir/DISABLED.txt");
	$engine->{lockfile} = (-f "$dir/archive_active.lck");
	($desc, $started, $channels, $connected) =
	    check_engine($localhost, $engine->{port});
	if (length($started) > 0)
	{
	    $engine->{started} = $started;
	    $engine->{channels} = $channels;
	    $engine->{connected} = $connected;
	    print("$engine->{desc} running since $engine->{started}\n")
		if ($opt_d);
	}
	else
	{
	    print("$engine->{desc} NOT running\n")
		if ($opt_d);
	    $engine->{started} = '';
	}
    }
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

# Check if now's the time to stop an engine
sub stop_engines($)
{
    my ($now) = @ARG;
    my ($engine);
    my ($stopped) = 0;
    print(" ---- stop engines\n") if ($opt_d);
    foreach $engine ( @config )
    {
	next if ($engine->{next_stop} <= 0); # never
	next if ($engine->{next_stop} > $now); # not, yet

	print "$engine->{desc} to stop at ",
	time_as_text($engine->{next_stop}),  "\n"
	    if ($opt_d);

	stop_engine($localhost, $engine->{port});
	$engine->{started} = $engine->{lockfile} = '';
	++ $stopped;
    }
    return $stopped;
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
	   "Starting Engine '$engine->{desc}': $localhost:$engine->{port}\n");
	my ($path) = dirname("$dir/$index");
	if (not -d $path)
	{
	    add_message("Creating dir '$path'\n");
	    mkpath($path);
	}
	my ($EngineLog) = time_as_short_text($now) . ".log";
	my ($cmd) = "cd \"$dir\";" .
	    "$ArchiveEngine -d \"$engine->{desc}\" -l $EngineLog " .
	    "-p $engine->{port} $cfg $index >$null 2>&1 &";
	print(time_as_text(time), ": Command: '$cmd'\n");
	system($cmd);
	# Create symlink from this new index ('2005/03_21/index')
	# to 'current index' in the engine directory
	# for quicker retrieval of current data
	# when there is no need to use the master_index
	# which gets updated later.
	my ($here) = readlink("$dir/current_index");
        # if link exists, remove it
	if (defined($here)) 
	{
	   unlink("$dir/current_index");
        }
        symlink("$dir/$index", "$dir/current_index");
	if (add_index("$dir/$index"))
	{
	    write_indexconfig(1);
	    return 1;
	}
	return 0;
}

# Attempt to start all engines that have their start time set
sub start_engines($)
{
    my ($now) = @ARG;
    my ($engine, $index_changed);
    print(" ---- start engines ", time_as_text($now),"\n") if ($opt_d);
    $index_changed = 0;
    foreach $engine ( @config )
    {
	next if ($engine->{disabled});                # ignore while disabled
	next unless ($engine->{next_start} > 0);      # not scheduled
	next unless ($engine->{next_start} <= $now);  # not due, yet.
	if ($engine->{lockfile})
	{
	    print "Cannot start engine $engine->{desc}: locked\n";
	    next;
	}
	$index_changed += start_engine($now, $engine);
    }
    if ($index_changed > 0)
    {
	run_indextool(time());
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
    print("\t-p <port>    : TCP port number for HTTPD\n");
    print("\t-f <file>    : config. file\n");
    print("\t-i <URL>     : path or URL to indexconfig.dtd\n");
    print("\t-u <minutes> : Update period for master index\n");
    print("\t-d           : debug mode (stay in foreground etc.)\n");
    print("\n");
    print("This tool automatically starts, monitors and restarts\n");
    print("ArchiveEngines based on a config. file.\n");
    exit(-1);
}
if (!getopts('hp:f:i:u:d')  ||  $#ARGV != -1  ||  $opt_h)
{
    usage();
}
if ($opt_d)
{
    print("Debug mode, will run in foreground\n");
    $daemonization = 0;
}

# Allow command line options to override various defaults
$http_port = $opt_p if ($opt_p);
$config_file = $opt_f if ($opt_f);
if (length($config_file) <= 0)
{
    print("Need config file (option -f):\n");
    usage();
}
read_config($config_file);
if (length($index_config) > 0  and -f $index_config)
{
    my ($dtd) = get_DTD($index_config);
    $master_index_dtd = $dtd if (length($dtd) > 0);
    read_indexconfig($index_config);
}  
$master_index_dtd = $opt_i if (defined($opt_i) and length($opt_i) > 0);
$index_update_minper = $opt_u if (defined($opt_u) and $opt_u > 0);
add_message("Started");
print("Read $config_file. Check status via\n");
print("          http://$localhost:$http_port\n");
print("You can also monitor the log file:\n");
print("          $logfile\n");
# Daemonization, see "perldoc perlipc"
if ($daemonization)
{
    print("Disassociating from terminal.\n");
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
write_indexconfig(1);
while (1)
{
    $now = time;
    if (($now - $last_check) > $engine_check_secper)
    {
	print("\n############## Main Loop Check ####\n") if ($opt_d);
	check_engines($now);
	update_schedule($now);
	start_engines($now);
	if (stop_engines($now) > 0)
	{
	    # We stopped engines. Check again soon for restarts.
	    $last_check += 10;
	}
	else
	{
	    $last_check = time;
	}
    }
    if (($now - $last_index_update) > ($index_update_minper*60))
    {
	write_indexconfig(0);
	run_indextool($now);
    }
    check_HTTPD($httpd);
}
