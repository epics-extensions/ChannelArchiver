# Example on how to link the non-shared library


BASE=../../../../../base
cd O.$HOST_ARCH
g++ -o CGIExport -g    -I $BASE/src/ca -Wall -DLINUX -D_X86_ -Dlinux -D_USE_BSD   -DUNIX  -DEXPL_TEMPL -I. -I..  -I../../../../include -I../../../../include/os/$HOST_ARCH -I../../../../../extensions/include -I$BASE/include -I$BASE/include/os/$HOST_ARCH         -L$BASE/lib/$HOST_ARCH/ -L ../../../../lib/$HOST_ARCH CGIInput.o HTMLPage.o main.o -lChanArchIO -lTools $BASE/lib/$HOST_ARCH/libca.a $BASE/lib/$HOST_ARCH/libCom.a -lpthread   -lm        
