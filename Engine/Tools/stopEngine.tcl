# Simple example how engine can be stopped via tcl.
# - And since tcl can be called from a script:
#   Example how to influence the engine from scripts ;-)
#
# -kuk-

# Configure this:
#
set machine "localhost"
set port 4812
set user "engine"
set pass "password"

##############################
package require http

set url "http://$machine:$port/stop?USER=$user&PASS=$pass"

set engine [ ::http::geturl $url -timeout 2000 ]

set status [ ::http::status $engine ]
if { ! [ string match $status "ok" ] } {
	error "Cannot connect to ArchiveEngine at $url, status: $status"
}


