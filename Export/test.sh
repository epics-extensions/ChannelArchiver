#!/bin/sh

# Use the tool from this source tree,
# not one that happens to be in the PATH.
EXPORT=O.$EPICS_HOST_ARCH/ArchiveExport

function check()
{
    output=test/$1
    previous=test/$1.OK
    info=$2
    diff -q $output $previous
    if [ $? -eq 0 ]
    then
        echo "OK : $info"
    else
        echo "FAILED : $info. Check diff $output $previous"
        exit 1
    fi
}

echo ""
echo "Export Test"
echo "***********"

INDEX=../DemoData/index 

# Test the sampled data
$EXPORT $INDEX fred >test/fred
check fred "Full single channel dump."

$EXPORT $INDEX fred janet >test/fred_janet
check fred_janet "Full dual channel dump."

$EXPORT $INDEX -s '03/23/2004 10:48:39' -e '03/23/2004 10:49:10' fred janet >test/fred_janet_piece
check fred_janet_piece "Time range of dual channel dump."

$EXPORT $INDEX -o test/raw -gnuplot fred janet
check raw "GNUPlot data file."

$EXPORT $INDEX -o test/lin -gnuplot fred janet -linear 1.0
check lin "GNUPlot data file for 'linear'."

$EXPORT $INDEX -o test/avg -gnuplot fred janet -average 5.0
check avg "GNUPlot data file for 'average'."

gnuplot test/combined.plt


cd test
#rm fred fred_janet fred_janet_piece raw raw.plt lin lin.plt avg avg.plt

