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
source casiTools.tcl

set archiveName "../../Engine/Test/freq_directory"
set channelName "fred"
set archive [ archive ]
set channel [ channel ]
set value   [ value ]
$archive open $archiveName
$archive findChannelByName $channelName $channel
$channel getFirstValue $value
while { [ $value valid ] } {
	puts [ formatValue $value ]
	$value next
}
