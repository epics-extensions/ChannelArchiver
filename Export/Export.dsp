# Microsoft Developer Studio Project File - Name="Export" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Export - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Export.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Export.mak" CFG="Export - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Export - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Export___Win32_Debug"
# PROP BASE Intermediate_Dir "Export___Win32_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\LibIO" /I "..\..\Tools" /I "..\..\..\base\include" /I "..\..\..\base\include\os\WIN32" /I "..\..\..\base\src\ca" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D CPP_EDITION=3 /D "OLD_osiTime" /D "EPICS_DLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\LibIO" /I "..\..\Tools" /I "..\..\..\..\base\include" /I "..\..\..\..\base\include\os\WIN32" /I "..\..\..\..\base\src\ca" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D CPP_EDITION=3 /D "OLD_osiTime" /D "EPICS_DLL" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ca.lib Com.lib db.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/ArchiveExport.exe" /pdbtype:sept /libpath:"..\..\..\base\lib\WIN32"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ca.lib Com.lib db.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /out:"Debug/ArchiveExport.exe" /pdbtype:sept /libpath:"..\..\..\..\base\lib\WIN32"
# Begin Target

# Name "Export - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\LibIO\ArchiveException.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ArchiveI.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\ArgParser.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\ASCIIParser.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\BinArchive.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\BinChannel.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\BinChannelIterator.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\BinCtrlInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\BinValue.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\BinValueIterator.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ChannelI.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ChannelIteratorI.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\CtrlInfoI.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\DataFile.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\DirectoryFile.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ExpandingValueIteratorI.cpp
# End Source File
# Begin Source File

SOURCE=.\Export.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\Exporter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\Filename.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\GenericException.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\gnu_regex.c
# End Source File
# Begin Source File

SOURCE=..\LibIO\GNUPlotExporter.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\HashTable.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\LinInterpolValueIteratorI.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\MsgLogger.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\MultiArchive.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\MultiChannel.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\MultiChannelIterator.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\MultiValueIterator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\osiTimeHelper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\RegularExpression.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\SpreadSheetExporter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\stdString.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ValueI.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ValueIteratorI.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Tools\ArgParser.h
# End Source File
# Begin Source File

SOURCE=.\Exporter.h
# End Source File
# Begin Source File

SOURCE=..\LibIO\Exporter.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\FilenameTool.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\GenericException.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\gnu_regex.h
# End Source File
# Begin Source File

SOURCE=..\LibIO\GNUPlotExporter.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\MemoryBuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\MsgLogger.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\osiTimeHelper.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\RegularExpression.h
# End Source File
# Begin Source File

SOURCE=..\LibIO\SpreadSheetExporter.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
