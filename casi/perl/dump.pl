#!/usr/bin/perl

# Perl must be able to find the shared library (e.g. casi.so, casi.dll).
# In addition, further EPICS base libs. might be needed (ca, Com).
# On Unix, set LD_LIBRARY_PATH
use lib "O." . $ENV{EPICS_HOST_ARCH};
use English;
use casi;
use strict;
use vars;

print "CASI Version: $casi::casi_version\n";

# Create archive, channel, value and open archive
my($a) = casi::new_archive();
my($c) = casi::new_channel();
my($v) = casi::new_value();
casi::archive_open($a, "../../Engine/Test/index");

# Dump all channel names
casi::archive_findFirstChannel($a, $c);
print "Channels:\n";
while (casi::channel_valid($c))
{
    print "\t" . casi::channel_name($c) . "\n";
    casi::channel_next($c);
}

# Dump values for single channel
my($name) = "fred";
casi::archive_findChannelByName($a, $name, $c)
    or die "Cannot find channel $name\n";

casi::channel_getFirstValue($c, $v);
while (casi::value_valid($v))
{
    print(casi::value_time($v) . " " . 
	  casi::value_text($v) . " " .
	  casi::value_status($v) . "\n");
    casi::value_next($v);
}

print("------------------------------\n");

# Dump a specific time range
my($start) = "2000/03/23 10:16:00";
my($end)   = "2000/03/23 10:19:00";

print "\nValues from $start to $end\n";
casi::channel_getValueAfterTime($c, $start, $v);
while (casi::value_valid($v)  and  casi::value_time($v) le $end)
{
    print(casi::value_time($v) . " " .
	  casi::value_text($v) . " " .
	  casi::value_status($v) . "\n");
    casi::value_next($v);
}

# Cleanup
casi::delete_value($v);
casi::delete_channel($c);
casi::delete_archive($a);

# With RedHat 8 & EPICS Base 3.14.1,
# the code from "my($a) = casi::new_archive();"
# until here was run in an infinite loop
# (well, I stopped it after 30min)
# and revealed no mem. leaks.
# Without the delete_* calls, memory of course grew.

