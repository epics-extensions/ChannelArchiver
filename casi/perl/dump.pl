#!/usr/bin/perl

use English;

# Set search path for the Perl module (casi.pm)
use lib "/home/kasemir/Epics/extensions/src/ChannelArchiver/casi/perl/O.Linux";

# For this to succeed, the OS must be able to find the
# shared library (e.g. casi.so, casi.dll).
# In addition, further EPICS base libs. might be needed (ca, Com).
# On Unix, set LD_LIBRARY_PATH

use casi;
use strict;
use vars qw($a $c $v $name);

print "CASI Version: $casi::casi_version\n";

# Create archive, channel, value, open archive
$a = archive::new();
$c = channel::new();
$v = value::new();
$a->open("../../Engine/Test/freq_directory");

# Dump all channel names
$a->findFirstChannel($c);
print "Channels:\n";
while ($c->valid())
{
    print "\t" . $c->name() . "\n";
    $c->next();
}

# Dump values for single channel
$name = "fred";
$a->findChannelByName($name, $c)
    or die "Cannot find channel $name\n";

$c->getFirstValue($v);
while ($v->valid())
{
    print $v->time() . " " . $v->text() . " " . $v->status() . "\n";
    $v->next();
}


# Dump a specific time range
my ($start, $end);

$start = "2000/03/23 10:16:00";
$end   = "2000/03/23 10:19:00";

print "\nValues from $start to $end\n";
$c->getValueAfterTime($start, $v);
while ($v->valid()  and  $v->time() le $end)
{
    print $v->time() . " " . $v->text() . "\n";
    $v->next();
}
