if [ $# -ne 1 ]
then
    echo "Usage: cgitest.sh request-file"
    exit 1
fi

REQUEST="$1";

export REQUEST_METHOD=POST
export CONTENT_TYPE=text/xml
export CONTENT_LENGTH=`wc -c <$REQUEST`
export SERVERCONFIG=`pwd`/test.xml
echo "Length: $CONTENT_LENGTH"
cat $REQUEST | O.${EPICS_HOST_ARCH}/ArchiveDataServer

#cat /tmp/archserver.log
#rm /tmp/archserver.log


