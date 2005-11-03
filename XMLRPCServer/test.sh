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
# stay the same when the test runs at other sites:
sh cgitest.sh request.info             |
   grep -v "^built "                   |
   grep -v "^from sources for version" |
   grep -v "^Config: "  >test/info
compare info "archiver.info command"


sh cgitest.sh request.archives >test/archives
compare archives "archiver.archives command"

sh cgitest.sh request.names >test/names
compare names "archiver.names command"

sh cgitest.sh request.data >test/data
compare data "archiver.data command"

