# Simple example how engine can be stopped via python.
# - And since python can be called from a script:
#   Example how to influence the engine from scripts ;-)
#
# -kuk-

# Configure this:
#
machine="localhost"
port=4812
# leave user empty if you do not use the user/passwd mechanism
user="engine"
user = ""
password="password"


##############################
import urllib, sys, regex

if user:
    url="http://%s:%d/stop?USER=%s&PASS=%s" % (machine, port, user, password)
else:
    url="http://%s:%d/stop" % (machine, port)

try:
    url=urllib.urlopen (url)
except IOError, message:
    print "Cannot connect, error:", message
    sys.exit (-1)

page=url.read()
if regex.search ("Engine Stopped", page) >= 0:
    print "Engine stopped"
else:
    print "Cannot stop Engine, this is the raw page I got:"
    print page



