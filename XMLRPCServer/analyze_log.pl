# Get some more or less useful stats from the data server's log file.
# Of course this requires that log file to be generated,
# whic is a compile-time option.

use English;
use strict;

# Response time groups for the 'Histogram'.
# Might need adjustment for your site.
my (@runtime_groups) = ( 0.005, 0.01, 0.1, 0.5, 1.0, 2.0, 3.0, 5.0, 8.0, 10.0, 20.0, 30.0, 60.0, 120.0, 240.0 );

my ($logfile) = "/tmp/archserver.log";


my (@runtimes, $i);
for ($i=0; $i<=$#runtime_groups; ++$i)
{
    push @runtimes, 0;
}
my ($outrageous)     = 0;
my ($most_outrageous)= 0;
my ($total_runs)     = 0;

sub register_runtime($)
{
    my ($time) = @ARG;
    my ($i);

    ++$total_runs;
    for ($i=0; $i<=$#runtime_groups; ++$i)
    {
        if ($time <= $runtime_groups[$i])
        {
            ++$runtimes[$i];
            return;
        }
    }
    ++$outrageous;
    if ($time > $most_outrageous)
    {
        $most_outrageous = $time;
    }
}

open(LOG, $logfile) or die "Cannot read log file '$logfile'";
while (<LOG>)
{
    if (m/.*ArchiveServer ran ([0-9.]+) seconds/)
    {
        my ($secs) = $1;
	register_runtime($secs);
    }
}

print "Archive Data Server Runtime Statistics\n";
print "\n";
printf "Response [secs]  Times   Percentage of total\n",
my ($i);
for ($i=0; $i<=$#runtime_groups; ++$i)
{
    printf "%7.3f         %5d     %5.1f %%\n",
           $runtime_groups[$i],
           $runtimes[$i],
           100.0 * $runtimes[$i] / $total_runs;
}
print "Outragerous runtimes: $outrageous\n";
print "maximum: $most_outrageous\n";
print "Total number of requests: $total_runs\n";

