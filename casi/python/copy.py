# $id$
#
# Simple all values, all channels copy demo.
#
# Use e.g.
#   ArchiveManager ../../Engine/Test/freq_directory -Compare /tmp/dir
# for verification
#
# Benchmark result:
#
# On a machine were Engine/bench.cpp can write  ~7700 values/sec,
# this script could copy (read, write, do python) ~2200 val/sec

import casiTools, casi, time

old='../../Engine/Test/freq_directory'
new='/tmp/dir'
hours_per_file=casi.HOURS_PER_MONTH
hours_per_file=0.5

# Open both archives, channels, ...
archive0 = casi.archive()
channel0 = casi.channel()
value0   = casi.value()
archive1 = casi.archive()
channel1 = casi.channel()

if not archive0.open (old):
    raise IOError, "cannot open %s" % old

if not archive1.write (new, hours_per_file):
    raise IOError, "cannot open %s" % old

# Loop channels

# This could be "findChannelByPattern"
archive0.findFirstChannel (channel0)
total = 0
t0 = time.clock()
while channel0.valid():
    name  = channel0.name()
    start = channel0.getFirstTime()
    end   = channel0.getLastTime()
    print "Channel: ", name
    print "\t     start:", start
    print "\t     end  :", end
    # Test if this channel & time range should be copied...

    # Check channel in new archive
    if archive1.findChannelByName (name, channel1):
        print "\talready in new archive"
        if start < channel1.getLastTime():
            start = channel1.getLastTime()
            print "\tadj. t0:", start
            # "after" is >=,
            # e.g. we might have to skip the exact same time stamp
            # to avoid duplication
            channel0.getValueAfterTime (start, value0)
            if value0.valid() and value0.time() == start:
                value0.next()
        else:
            channel0.getFirstValue (value0)
    else:
        archive1.addChannel (name, channel1)
        print "\tadded to new archive"
        channel0.getFirstValue (value0)

    # Value loop
    values = 0
    while value0.valid() and value0.time() <= end:
        # Right here many tests are missing:
        # * valid time stamp
        # * going back in time?
        # * repeat count OK?
        # * ....
        
        if not channel1.addValue (value0):
            # add new buffer
            next_file_time = archive1.nextFileTime (value0.time())
            count = value0.determineChunk (next_file_time)
            channel1.addBuffer (value0, count)
            # try again:
            if not channel1.addValue (value0):
                raise IOError, "cannot add value"

        values = values + 1
        value0.next ()

    print "\t%d values" % values
    channel1.releaseBuffer ()
    channel0.next()
    total = total + values

t1 = time.clock()
elapsed = t1 - t0

if elapsed:
    print "Average:", total/elapsed, "val/sec"
else:
    print "Total:", total, "values"


