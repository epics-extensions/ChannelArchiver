# Example on how to link the non-shared library

BASE=/cs/epics/R3.13.3/base

echo "Building O.$HOST_ARCH/CGIExport..."
cd O.$HOST_ARCH
g++ -o CGIExport \
	-g -Wall -DLINUX -D_X86_ -Dlinux -D_USE_BSD   -DUNIX  -DEXPL_TEMPL\
	-I $BASE/src/ca \
	-I. -I.. \
	-I../../../../include -I../../../../include/os/$HOST_ARCH\
	-I../../../../../extensions/include \
	-I$BASE/include -I$BASE/include/os/$HOST_ARCH   \
	-L$BASE/lib/$HOST_ARCH/ -L ../../../../lib/$HOST_ARCH\
	CGIInput.o HTMLPage.o main.o\
	-lChanArchIO -lTools $BASE/lib/$HOST_ARCH/libca.a $BASE/lib/$HOST_ARCH/libCom.a -lpthread   -lm        
cd ..

echo "Installing as Tests/cgi/CGIExport.cgi"
cp O.$HOST_ARCH/CGIExport Tests/cgi/CGIExport.cgi
chmod +x Tests/cgi/CGIExport.cgi














