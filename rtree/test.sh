
rm -f test/file_allocator.dat
O.$EPICS_HOST_ARCH/file_allocator_test >test/file_allocator.out
diff test/file_allocator.out test/file_allocator.OK
if [ $? -eq 0 ]
then
	echo "OK: FileAllocator"
else
	echo "FAILED FileAllocator test, check test/file_allocator.out"
fi
