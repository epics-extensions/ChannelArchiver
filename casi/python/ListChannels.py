# --------------------------------------------------------
# $Id$
#
# Please refer to NOTICE.txt,
# included as part of this distribution,
# for legal information.
#
# Kay-Uwe Kasemir, kasemir@lanl.gov
# --------------------------------------------------------

import casi
import sys

def listChannels (archiveName, channelPattern=""):
	"List all channel names that match the pattern"
	archive = casi.archive()
	channel = casi.channel()
	if not archive.open(archiveName):
		print "Cannot open", archiveName
		sys.exit()
    
	archive.findChannelByPattern(channelPattern, channel)
	while channel.valid():
		print channel.name()
		channel.next()

def usage():
	print "USAGE: " + sys.argv[0] + " archive [ channelPattern ]"
	sys.exit (1)

if __name__=="__main__":
	argc=len(sys.argv)
	if argc < 2: usage ()
	apply (listChannels, sys.argv[1:])

