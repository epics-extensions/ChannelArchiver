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
import _casi

# Simply importing this SWIG-generated file makes python core-dump.
# Without importing it, all is fine but we lack the OO-wrappers,
# we only have the _casi function interface
#import casi

def test(archiveName, channelName):
    a = _casi.new_archive()
    c = _casi.new_channel()
    v = _casi.new_value()
    _casi.archive_open(a, archiveName)
    _casi.archive_findChannelByName(a, channelName, c)
    _casi.channel_getFirstValue(c, v)
    while _casi.value_valid(v):
        print _casi.value_text(v)
        _casi.value_next(v)
    _casi.delete_value(v)
    _casi.delete_channel(c)
    _casi.delete_archive(a)

for i in range(100):
    test("../../Engine/Test/index","fred")
print "I do get here"
