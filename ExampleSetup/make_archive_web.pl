#!/usr/bin/perl
# make_archive_web.pl
#
# Create web page with archive info
# (daemons, engines, ...)
# from tab-delimited configuration file.

BEGIN { push(@INC, '/arch/scripts'); }

use English;
use strict;
use vars qw($opt_d $opt_h $opt_c $opt_o);
use Getopt::Std;
use Data::Dumper;
use Sys::Hostname;
use archiveconfig;

# Globals, Defaults
my ($config_name) = "archiveconfig.xml";
my ($output_name) = "archive_status.html";
my ($localhost) = hostname();

# Configuration info filled by parse_config_file
my (@daemons);

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

sub write_html($)
{
    my ($filename) = @ARG;
    my $TITLE = "ARCHIVER HEALTH STATUS";
    my ($daemon, $engine);
    my ($out, $disconnected);
    open($out, ">$filename") or die "Cannot open '$filename'\n";

    print $out <<"XML";
<html>
<head></head>

<body BGCOLOR=#A7ADC6 LINK=#0000FF VLINK=#0000FF ALINK=#0000FF>

<H1 align=center>$TITLE</H1>

<table border="1" cellpadding="1" cellspacing="1" style'"border-collapse: collapse" bordercolor="#111111" bgcolor="#CCCCFF" width="100%"'>

  <tr>
     <td width="15%" align="center"><b>DAEMON</b></td>
     <td width="10%" align="center"><b>ENGINE</b></td>
     <td width="5%"  align="center"><b>PORT</b></td>
     <td width="20%" align="center"><b>DESCRIPTION</b></td>
     <td width="35%" align="center"><b>STATUS</b></td>
     <td width="10%" align="center"><b>RESTART</b></td>
     <td width="5%" align="center"><b>TIME</b></td>
  </tr>
XML

    foreach $daemon ( @daemons )
    {
        print $out "  <tr>\n";
        print $out "     <td width=\"15%\"><A HREF=\"http://$localhost:$daemon->{port}\">$daemon->{name}</A></td>\n";
        print $out "     <td width=\"10%\">&nbsp;</td>\n";
        print $out "     <td width=\"5%\">$daemon->{port}</td>\n";
        print $out "     <td width=\"20%\">$daemon->{desc}</td>\n";
        if ($daemon->{running})
        {
            print $out "     <td width=\"35%\">Running</td>\n";
        }
        else
        {
            print $out "     <td width=\"35%\"><FONT color=#FF0000>Down</FONT></td>\n";
        }
        print $out "     <td width=\"5%\">&nbsp;</td>\n";
        print $out "     <td width=\"10%\">&nbsp;</td>\n";
        print $out "  </tr>\n";
        foreach $engine ( @{ $daemon->{engines} } )
        {
            print $out "  <tr>\n";
            print $out "     <td width=\"15%\">&nbsp;</td>\n";
            print $out "     <td width=\"10%\"><A HREF=\"http://$localhost:$engine->{port}\">$engine->{name}</A></td>\n";
            print $out "     <td width=\"5%\">$engine->{port}</td>\n";
            print $out "     <td width=\"20%\">$engine->{desc}</td>\n";
            if ($engine->{status} eq "running")
            {
                $disconnected = $engine->{channels} - $engine->{connected};
                if ($disconnected == 0)
                {
                    print $out "     <td width=\"35%\">$engine->{channels} channels connected.</td>\n";
                }
                else
                {
                    print $out "     <td width=\"35%\">$engine->{channels} channels, <FONT color=#FF0000>$disconnected disconnected</FONT>.</td>\n";
                }
            }
            else
            {
                print $out "     <td width=\"35%\"><FONT color=#FF0000>$engine->{status}</FONT></td>\n";
            }
            print $out "     <td width=\"10%\">$engine->{restart}</td>\n";
            print $out "     <td width=\"5%\">$engine->{time}</td>\n";
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

@daemons = parse_config_file($config_name, $opt_d);
update_status(\@daemons, $opt_d);
dump_config(\@daemons) if ($opt_d);
write_html($output_name);
