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

# Dump all values for some channel, no error checking at all
import casiTools, casi, sys

archiveName, channelName=("../../Engine/Test/index","fred")

archive = casi.archive()
channel = casi.channel()
value   = casi.value()
archive.open (archiveName)
archive.findChannelByName (channelName, channel)
channel.getFirstValue (value)
while value.valid():
    print casiTools.formatValue (value)
    value.next()

