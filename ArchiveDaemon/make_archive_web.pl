#!/usr/bin/perl
# make_archive_web.pl
#
# This file creates a web page with archive info
# (daemons, engines, ...)
# from tab-delimited configuration file.

use English;
use strict;
use Socket;
use IO::Handle;
use Sys::Hostname;
use vars qw($opt_d $opt_h $opt_c $opt_o);
use Getopt::Std;
use Data::Dumper;

# Globals, Defaults
my ($config_name) = "archiveconfig.csv";
my ($output_name) = "archive_status.html";
my ($localhost) = hostname();

# Timeout used when reading a HTTP client or ArchiveEngine.
# 10 seconds is reasonable.
my ($read_timeout) = 10;

# Configuration info filled by parse_config_file
my (@daemons);

# Array of daemons to run, their info, and their engines:
# 
# $daemons[0]->{name} = "demosys";     # Daemon name (directory)
# $daemons[0]->{desc} = "Test daemon"; # .. description
# $daemons[0]->{port} = 4000;          # .. port
# $daemons[0]->{status} = "Down";      # .. status
# .. and the engines under this daemon w/ their name, desc, port, ...
# $daemons[0]->{engines}[0]->{name} = "engine1";
# $daemons[0]->{engines}[0]->{desc} = "Test Engine 1";
# $daemons[0]->{engines}[0]->{port} = 4001;
# Debug/Verify Data Layout
# print Dumper(\@daemons);

sub usage()
{
    print("USAGE: make_archive_web [options]\n");
    print("\n");
    print("Options:\n");
    print(" -h          : help\n");
    print(" -c <config> : Use given config file instead of $config_name\n");
    print(" -o <html>   : Generate given html file instead of $output_name\n");
    print(" -d          : debug\n");
}

sub time_as_text($)
{
    my ($seconds) = @ARG;
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)
	= localtime($seconds);
    return sprintf("%04d/%02d/%02d %02d:%02d:%02d",
		   1900+$year, 1+$mon, $mday, $hour, $min, $sec);
}

sub parse_config_file($)
{
    my ($filename) = @ARG;
    my ($in);
    my ($type, $name, $port, $desc);
    my ($di, $ei); # Index of current daemon and engine
    open($in, $filename) or die "Cannot open '$filename'\n";
    print("Reading $filename\n") if ($opt_d);
    while (<$in>)
    {
	chomp;                          # Chop CR/LF
	next if ($ARG =~ '\A#');        # Skip comments
	next if ($ARG =~ '\A[ \t]*\Z'); # ... and empty lines
	($type,$name,$port,$desc) = split(/\t/, $ARG); # Get columns
	$desc = $name unless (length($desc) > 0); # Desc defaults to name
	if ($type eq "DAEMON")
	{
	    print("$NR: Daemon '$name', Port $port, Desc '$desc'\n") if ($opt_d);
	    $di = $#daemons + 1;
	    $daemons[$di]->{name} = $name;
	    $daemons[$di]->{desc} = $desc;
	    $daemons[$di]->{port} = $port;
	    $daemons[$di]->{status} = "unknown";
	    $ei = 0;
	}
	elsif ($type eq "ENGINE")
	{
	    print("$NR: Engine '$name', Port $port, Desc '$desc'\n") if ($opt_d);
	    $daemons[$di]->{engines}[$ei]->{name} = $name;
	    $daemons[$di]->{engines}[$ei]->{desc} = $desc;
	    $daemons[$di]->{engines}[$ei]->{port} = $port;
	    ++ $ei;
	}
	else
	{
	    die("File '$filename', line $NR: Cannot handle type '$type'\n");
	}
    }
    close $in;
    print("Read $filename\n\n") if ($opt_d);
}

sub dump_config()
{
    my ($daemon, $engine);
    print("Configuration Dump:\n");
    foreach $daemon ( @daemons )
    {
	print("Daemon '$daemon->{name}'");
	print(" Port $daemon->{port}, '$daemon->{desc}'\n");
	foreach $engine ( @{ $daemon->{engines} } )
	{
	    print("    Engine '$engine->{name}'");
	    print(" Port $engine->{port}, '$engine->{desc}'\n");
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

sub update_status()
{
    my ($daemon);
    my (@html, $engines, $channels);
    foreach $daemon ( @daemons )
    {
	@html = read_URL($localhost, $daemon->{port}, "/status");
	print "Response from $daemon->{desc}:\n" if ($opt_d);
	print @html if ($opt_d);
	if ($#html < 14)
        {
            $daemon->{status} = "<font color=#FF0000>NOT RUNNING</font>";
        }
	elsif ($html[8] =~ "Archive Daemon")
        {
            # 3 of 3 engines are running<p>
            if ($html[14] =~ "(.+)<p>")
            {
                $engines = $1;
	        if ($html[15] =~ "([0-9]+) of ([0-9]+) channels")
                {
                     $daemon->{status} = "$engines<br>$1 of $2 channels connected";
                }
            }
            else
            {
                $daemon->{status} = "<font color=#FF0000>CHECK ENGINES</font>";
            }
        }
        else
        {
            $daemon->{status} = "<font color=#FF0000>CHECK DEAMON</font>";
        }
    }
}

sub write_html($)
{
    my ($filename) = @ARG;
    my $TITLE = "ARCHIVER HEALTH STATUS";
    my ($daemon, $engine);
    my ($out);
    open($out, ">$filename") or die "Cannot open '$filename'\n";

    print $out <<"XML";
 <html>
 <head></head>

 <body BGCOLOR=#A7ADC6 LINK=#0000FF VLINK=#0000FF ALINK=#0000FF>

 <H1 align=center>$TITLE</H1>

<table border="1" cellpadding="1" cellspacing="1" style'"border-collapse: collapse" bordercolor="#111111" bgcolor="#CCCCFF" width="100%"'>

  <tr>
     <td width="15%" align="center"><b>DAEMON</b></td>
     <td width="15%" align="center"><b>ENGINE</b></td>
     <td width="5%"  align="center"><b>PORT</b></td>
     <td width="25%" align="center"><b>DESCRIPTION</b></td>
     <td width="40%" algin="center"><b>STATUS</b></td>
  </tr>
XML

    foreach $daemon ( @daemons )
    {
        print $out "  <tr>\n";
        print $out "     <td width=\"15%\"><A HREF=\"http://$localhost:$daemon->{port}\">$daemon->{name}</A></td>\n";
        print $out "     <td width=\"15%\">&nbsp;</td>\n";
        print $out "     <td width=\"5%\">$daemon->{port}</td>\n";
        print $out "     <td width=\"25%\">$daemon->{desc}</td>\n";
        print $out "     <td width=\"40%\">$daemon->{status}</td>\n";
        print $out "  </tr>\n";
	foreach $engine ( @{ $daemon->{engines} } )
	{
            print $out "  <tr>\n";
            print $out "     <td width=\"15%\">&nbsp;</td>\n";
            print $out "     <td width=\"15%\"><A HREF=\"http://$localhost:$engine->{port}\">$engine->{name}</A></td>\n";
            print $out "     <td width=\"5%\">$engine->{port}</td>\n";
            print $out "     <td width=\"25%\">$engine->{desc}</td>\n";
            print $out "     <td width=\"40%\">&nbsp;</td>\n";
            print $out "  </tr>\n";
	}
    }

    print $out "<hr><p>\n";
    print $out "Last Update: ", time_as_text(time), "<p>\n";
    print $out "</table>\n";
    print $out "</body>\n";
    print $out "</html>\n";
 
    close $out;
}

# The main code ==============================================

# Parse command-line options
if (!getopts("dhc:o:") ||  $opt_h)
{
    usage();
    exit(0);
}
$config_name = $opt_c  if (length($opt_c) > 0);
$output_name = $opt_o  if (length($opt_o) > 0);

parse_config_file($config_name);
dump_config() if ($opt_d);
update_status();
write_html($output_name);
