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

