# Makedoc.bat:
#
# This is actually a WIN32 bat file and a UNIX shell script
# because MakeDoc.pl handles the filename globbing...
#

mkdir libio
cd libio
perl ../MakeDoc.pl -v ../../LibIO/*I.h ../../LibIO/BinArchive.h ../../LibIO/MultiArchive.h
#perl ../MakeDoc.pl -v ../../LibIO/*I.h ../../LibIO/BinArchive.h

cd ..
cd engine
perl ../MakeDoc.pl -v ../../Engine/Engine.h ../../Engine/Configuration.h ../../Engine/ConfigFile.h

cd ..
cd atac
perl ../MakeDoc.pl -v ../../atac/atac.cpp ../../atac/atacTools.tcl ../../atac/atacGraph.tk
cd ..
