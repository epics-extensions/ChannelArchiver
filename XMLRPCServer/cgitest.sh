if [ $# -ne 1 ]
then
    echo "Usage: cgitest.sh request-file"
    exit 1
fi

REQUEST="$1";

export REQUEST_METHOD=POST
export CONTENT_TYPE=text/xml
export CONTENT_LENGTH=`wc --bytes <$REQUEST`
export INDEX=/mnt/bogart_home/snsdoc/RF/HPRF/LANLXmtrData/2003/01xx/index
cat $REQUEST | O.linux-x86/ArchiveServer

cat /tmp/archserver.log
rm /tmp/archserver.log


