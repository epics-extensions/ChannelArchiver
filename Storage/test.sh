make && O.linux-x86/RTreeTest
dot -Tpng index0.dot -o index0.png
dot -Tpng index1.dot -o index1.png
pngtopnm index0.png >index0.pnm
pngtopnm index1.png >index1.pnm
pnmcat -tb index0.pnm index1.pnm >index.pnm
eog index.pnm &
exit 0

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
diff test/test_data2.dot test/test_data2.dot.OK
if [ $? -eq 0 ]
then
	echo "OK: RTree test_data2"
else
	echo "FAILED RTree test_data2, check test/test_data2.dot"
fi

