# Makedoc.bat:
#
# This is actually a WIN32 bat file and a UNIX shell script
# because MakeDoc.pl handles the filename globbing...
#

mkdir libio
cd libio
perl ../MakeDoc.pl -v ../../LibIO/*I.h ../../LibIO/BinArchive.h ../../LibIO/MultiArchive.h

cd ..
cd engine
perl ../MakeDoc.pl -v ../../Engine/Engine.h ../../Engine/Configuration.h ../../Engine/ConfigFile.h
