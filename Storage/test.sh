make
rm -f test/file_allocator.dat
O.$EPICS_HOST_ARCH/FileAllocatorTest >test/file_allocator.out
diff test/file_allocator.out test/file_allocator.OK
if [ $? -eq 0 ]
then
	echo "OK: FileAllocator"
else
	echo "FAILED FileAllocator test, check test/file_allocator.out"
fi

O.$EPICS_HOST_ARCH/NameHashTest
if [ $? -eq 0 ]
then
	echo "OK: NameHashTest"
else
        echo "FAILED NameHashTest"
fi

rm -f test/index1 test/index2
rm -f test/update.tst
O.$EPICS_HOST_ARCH/RTreeTest
if [ $? -eq 0 ]
then
	echo "OK: RTreeTest"
else
        echo "FAILED RTree Test"
fi
diff test/test_data1.dot test/test_data1.dot.OK
if [ $? -eq 0 ]
then
	echo "OK: RTree test_data1"
else
	echo "FAILED RTree test_data1, check test/test_data1.dot"
fi
diff test/update_data.dot test/update_data.dot.OK
if [ $? -eq 0 ]
then
	echo "OK: RTree update_data"
else
	echo "FAILED RTree update_data, check test/update_data.dot"
fi

O.$EPICS_HOST_ARCH/ReadTest ../DemoData/index fred >test/fred
diff test/fred.OK test/fred
if [ $? -eq 0 ]
then
        echo "OK: fred"
else
        echo "FAILED fred"
fi

O.$EPICS_HOST_ARCH/ReadTest ../DemoData/index alan >test/alan
diff test/alan.OK test/alan
if [ $? -eq 0 ]
then
        echo "OK: alan"
else
        echo "FAILED alan"
fi
                                                                                         
O.$EPICS_HOST_ARCH/ReadTest ../DemoData/index BoolPV >test/BoolPV
diff test/BoolPV.OK test/BoolPV
if [ $? -eq 0 ]
then
        echo "OK: BoolPV"
else
        echo "FAILED BoolPV"
fi
                                                                                         


# Comparison of last two updates:
#dot -Tpng -o0.png update0.dot
#dot -Tpng -o1.png update1.dot
#pngtopnm 0.png >0.pnm
#pngtopnm 1.png >1.pnm
#pnmcat -tb 0.pnm 1.pnm >u.pnm
#rm 0.png 1.png 0.pnm 1.pnm
#eog u.pnm

