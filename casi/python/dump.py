#! /bin/env python
# --------------------------------------------------------
# $Id$
#
# Please refer to NOTICE.txt,
# included as part of this distribution,
# for legal information.
#
# Kay-Uwe Kasemir, kasemir@lanl.gov
# --------------------------------------------------------

import casiTools
import casi
import sys

def dump (archiveName, channelName,
          startStamp=0, endStamp="9999/99/99 24:59:59"):
    "From archive, dump channel in [start, end] time range"
    arch = casi.archive()
    chan = casi.channel()
    val  = casi.value()
    if not arch.open (archiveName):
        print "Cannot open ", archiveName
        sys.exit(1)
    
    arch.findChannelByName (channelName, chan)
    if not chan.valid():
        print channelName + ": not found"
        sys.exit(1)

    if startStamp > 0:
        chan.getValueAfterTime (startStamp, val)
    else:
        print "First Sample: " + chan.getFirstTime()
        print "Last  Sample: " + chan.getLastTime()
        chan.getFirstValue (val)

    # Loop until end of archive or end time exceeded
    count=0
    while val.valid() and val.time() <= endStamp:
        print casiTools.formatValue (val)
        val.next()
        count=count+1
    # val, chan, arch deleted automatically
    return count

def usage():
    print "USAGE: " + sys.argv[0] + " archive channelName [ startTime [ endTime ] ]"
    print "       start/endTime as \"YYYY/MM/DD hh:mm:ss\" in 24h format"
    print ""
    print "- Dump values for given channel between start- and end time"
    print ""
    print "Example"
    print "-------"
    print "python dump.py ../../Engine/Test/freq_directory fred \"2000/03/23 10:16:09\" \"2000/03/23 10:18:54\""
    print "2000/03/23 10:16:09.800541744 0.4891"
    print "2000/03/23 10:16:11.803278963 0.5086"
    print "2000/03/23 10:16:13.806193020 0.4196"
    print "2000/03/23 10:16:14.806193020 Archive_Off"
    print "2000/03/23 10:18:35.469894785 0.3443"
    print "2000/03/23 10:18:52.424309954 0.4355"
    sys.exit (1)


if __name__ == "__main__":
   # Parse args
   argc=len(sys.argv)
   if argc < 3: usage()
   apply (dump, sys.argv[1:])

