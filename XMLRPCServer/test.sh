#!/bin/sh

function compare()
{
    name=$1
    info=$2
    diff -q test/${name}.OK test/${name}
    if [ $? -eq 0 ]
    then
        echo "OK : $info"
    else
        echo "FAILED : $info. Check test/${name}.OK against test/${name}"
	exit 1
    fi
}

echo ""
echo "XML-RPC Data Server Test"
echo "************************"
echo ""

# sed -e "s/DEMO_INDEX/..\/DemoData\/index/" test_config.template >test_config.xml

# Run the 'info' command,
# strip the build date etc. from the
# output to keep what's more likely to
# stay the same when the test runs at other sites.
#
# Hassle with versions of xmlrpc-c:
# Content-type might be only "text/xml" or also
# include charset="utf-8".
# 
# What I don't get: 'Content-Length' changes.
# Example for the 'info' response:
#   xmlrpc-c 0.9.9 under Fedora Core 2:    "Content-length : 4794"
#   xmlrpc-c 0.9.10 under MacOS X:         "Content-length : 4797"
#   xmlrpc-c 1.03.06 under RedHat EL WS 4: "Content-length : 4782"
# When I naively count the characters in the 'body' the response,
# I get 4639 characters, so all the above length are wrong?!
#
# For the 'archives' request, all the above setups agree to '685'
# for a length.

sh cgitest.sh request.info             |
   grep -v "^Content-length:"          |
   grep -v "^Content-type:"            |
   grep -v "^built "                   |
   grep -v "^from sources for version" |
   grep -v "^Config: "  >test/info
compare info "archiver.info command"

sh cgitest.sh request.archives | grep -v "^Content-type:" >test/archives
compare archives "archiver.archives command"

sh cgitest.sh request.names | grep -v "^Content-type:" >test/names
compare names "archiver.names command"

sh cgitest.sh request.data | grep -v "^Content-type:" >test/data
compare data "archiver.data command"

