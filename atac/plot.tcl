#######################################################################
# Demo of plotting from archive
# with BLT
#
# run this with wish or bltwish

source atacTools.tcl
package require BLT
namespace import blt::*

set archiveName "../../CATests/LEDA/dir"
set channelName "is_hvps_actv"

set archiveId [ archive open $archiveName ]
set channelId [ archive findChannelByName $archiveId is_hvps_actv ]

#  last 24h:
set endTime [ channel getLastTime $channelId ]
set startTime [ secs2stamp [ expr [ stamp2secs $endTime ] - 60*60*24 ] ]
set valueId [ channel getValueAfterTime $channelId $startTime ]

initGraphData $channelName data
while { [ value valid $valueId ] } {
	addGraphData  $valueId data
	value next $valueId
}

createAtacGraph .g
addToGraph .g data
pack .g
update
saveGraph PS .g "snapshot.ps"

