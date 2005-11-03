function compare()
{
    file1=$1
    file2=$2
    info=$3
    diff -q test/$file1 test/$file2
    if [ $? -eq 0 ]
    then
        echo "OK : $info"
    else
        echo "FAILED : $info. Check test/$file1 against test/$file2"
        exit 1
    fi
}

echo ""
echo "Storage Library Test"
echo "********************"
echo ""

make
rm -f test/file_allocator.dat
O.$EPICS_HOST_ARCH/FileAllocatorTest >test/file_allocator.out
compare file_allocator.out file_allocator.OK "FileAllocator"

O.$EPICS_HOST_ARCH/NameHashTest
if [ $? -eq 0 ]
then
	echo "OK: NameHashTest"
else
        echo "FAILED NameHashTest"
        exit 1
fi

rm -f test/index1 test/index2
rm -f test/update.tst
O.$EPICS_HOST_ARCH/RTreeTest
if [ $? -eq 0 ]
then
	echo "OK: RTreeTest"
else
        echo "FAILED RTree Test"
        exit 1
fi
compare test_data1.dot test_data1.dot.OK "RTree test_data1"
compare update_data.dot update_data.dot.OK "RTree update_data"

O.$EPICS_HOST_ARCH/ReadTest ../DemoData/index fred >test/fred
compare fred.OK fred "ReadTest"

O.$EPICS_HOST_ARCH/ReadTest ../DemoData/index alan >test/alan
compare alan.OK alan "Dump of PV alan"

O.$EPICS_HOST_ARCH/ReadTest ../DemoData/index BoolPV >test/BoolPV
compare BoolPV.OK BoolPV "BoolPV"

# Comparison of last two updates:
#dot -Tpng -o0.png update0.dot
#dot -Tpng -o1.png update1.dot
#pngtopnm 0.png >0.pnm
#pngtopnm 1.png >1.pnm
#pnmcat -tb 0.pnm 1.pnm >u.pnm
#rm 0.png 1.png 0.pnm 1.pnm
#eog u.pnm

