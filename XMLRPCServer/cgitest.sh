if [ $# -ne 1 ]
then
    echo "Usage: cgitest.sh request-file"
    exit 1
fi

REQUEST="$1";

export REQUEST_METHOD=POST
export CONTENT_TYPE=text/xml
export CONTENT_LENGTH=`wc --bytes <$REQUEST`
export SERVERCONFIG=serverconfig.xml
cat $REQUEST | O.linux-x86/ArchiveDataServer

#cat /tmp/archserver.log
#rm /tmp/archserver.log


