# Example on how to link the non-shared library

echo "Building O.$HOST_ARCH/CGIExport..."
cd O.$EPICS_HOST_ARCH
g++ -o CGIExport \
	-g -Wall -DLINUX -D_X86_ -Dlinux -D_USE_BSD   -DUNIX  -DEXPL_TEMPL\
	-I $EPICS_BASE_RELEASE/src/ca \
	-I. -I.. \
	-I../../../../include -I../../../../include/os/$HOST_ARCH\
	-I../../../../../extensions/include \
	-I$EPICS_BASE_RELEASE/include -I$EPICS_BASE_RELEASE/include/os/$HOST_ARCH   \
	-L$EPICS_BASE_RELEASE/lib/$HOST_ARCH/ -L ../../../../lib/$EPICS_HOST_ARCH\
	CGIInput.o HTMLPage.o main.o\
	-lChanArchIO -lTools $EPICS_BASE_RELEASE/lib/$EPICS_HOST_ARCH/libca.a $EPICS_BASE_RELEASE/lib/$EPICS_HOST_ARCH/libCom.a -lpthread   -lm        
cd ..

echo "Installing as Tests/cgi/CGIExport.cgi"
cp O.$EPICS_HOST_ARCH/CGIExport Tests/cgi/CGIExport.cgi
chmod +x Tests/cgi/CGIExport.cgi

