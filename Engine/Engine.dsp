# Microsoft Developer Studio Project File - Name="Engine" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Engine - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Engine.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Engine.mak" CFG="Engine - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Engine - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "Engine - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Engine - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /Zi /Od /I "..\LibIO" /I "..\..\Tools" /I "..\..\Tools\os\WIN32" /I "..\..\..\..\base\include" /I "..\..\..\..\base\include\os\Win32" /I "..\..\..\..\base\src\ca" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D OLD_osiTime=1 /D CPP_EDITION=3 /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib ca.lib Com.lib /nologo /subsystem:console /profile /debug /machine:I386 /out:"Debug/ArchiveEngine.exe" /libpath:"..\..\..\..\base\lib\WIN32" /libpath:"..\..\..\lib\WIN32"

!ELSEIF  "$(CFG)" == "Engine - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Engine___Win32_Release"
# PROP BASE Intermediate_Dir "Engine___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /Ob2 /I "..\LibIO" /I "..\..\Tools" /I "..\..\..\base\include" /I "..\..\..\base\include\os\Win32" /I "..\..\..\base\src\ca" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D OLD_osiTime=1 /D CPP_EDITION=3 /YX /FD /GZ /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /Ob2 /I "..\LibIO" /I "..\..\Tools" /I "..\..\..\..\base\include" /I "..\..\..\..\base\include\os\Win32" /I "..\..\..\..\base\src\ca" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D OLD_osiTime=1 /D CPP_EDITION=3 /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib ca.lib Com.lib /nologo /subsystem:console /profile /debug /machine:I386 /out:"Debug/ArchiveEngine.exe" /libpath:"..\..\..\base\lib\WIN32"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib ca.lib Com.lib /nologo /subsystem:console /profile /machine:I386 /out:"Debug/ArchiveEngine.exe" /libpath:"..\..\..\..\base\lib\WIN32"
# SUBTRACT LINK32 /debug

!ENDIF 

# Begin Target

# Name "Engine - Win32 Debug"
# Name "Engine - Win32 Release"
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

SOURCE=..\..\Tools\Bitset.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\CGIDemangler.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ChannelI.cpp
# End Source File
# Begin Source File

SOURCE=.\ChannelInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ChannelIteratorI.cpp
# End Source File
# Begin Source File

SOURCE=.\CircularBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\ConfigFile.cpp
# End Source File
# Begin Source File

SOURCE=.\Configuration.cpp
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

SOURCE=.\Engine.cpp
# End Source File
# Begin Source File

SOURCE=.\EngineServer.cpp
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

SOURCE=.\GroupInfo.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\HashTable.cpp
# End Source File
# Begin Source File

SOURCE=.\HTMLPage.cpp
# End Source File
# Begin Source File

SOURCE=.\HTTPServer.cpp
# End Source File
# Begin Source File

SOURCE=.\LockFile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\LowLevelIO.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\MsgLogger.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\NetTools.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\osiTimeHelper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\RegularExpression.cpp
# End Source File
# Begin Source File

SOURCE=.\ScanList.cpp
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
# Begin Source File

SOURCE=..\..\Tools\WIN32Thread.cpp
# End Source File
# Begin Source File

SOURCE=.\WriteThread.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\Tools\ArgParser.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\Bitset.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\CGIDemangler.h
# End Source File
# Begin Source File

SOURCE=.\ChannelInfo.cc
# End Source File
# Begin Source File

SOURCE=.\ChannelInfo.h
# End Source File
# Begin Source File

SOURCE=.\CircularBuffer.cc
# End Source File
# Begin Source File

SOURCE=.\CircularBuffer.h
# End Source File
# Begin Source File

SOURCE=.\Config.h
# End Source File
# Begin Source File

SOURCE=.\ConfigFile.h
# End Source File
# Begin Source File

SOURCE=.\Configuration.h
# End Source File
# Begin Source File

SOURCE=.\Engine.cc
# End Source File
# Begin Source File

SOURCE=.\Engine.h
# End Source File
# Begin Source File

SOURCE=.\EngineServer.cc
# End Source File
# Begin Source File

SOURCE=.\EngineServer.h
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

SOURCE=.\GroupInfo.h
# End Source File
# Begin Source File

SOURCE=.\HTMLPage.cc
# End Source File
# Begin Source File

SOURCE=.\HTMLPage.h
# End Source File
# Begin Source File

SOURCE=.\HTTPServer.cc
# End Source File
# Begin Source File

SOURCE=.\HTTPServer.h
# End Source File
# Begin Source File

SOURCE=.\LockFile.cc
# End Source File
# Begin Source File

SOURCE=.\LockFile.h
# End Source File
# Begin Source File

SOURCE=.\main.cc
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

SOURCE=.\ScanList.cc
# End Source File
# Begin Source File

SOURCE=.\ScanList.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\Thread.h
# End Source File
# Begin Source File

SOURCE=..\..\Tools\os\WIN32\WIN32Thread.h
# End Source File
# Begin Source File

SOURCE=.\WriteThread.cc
# End Source File
# Begin Source File

SOURCE=.\WriteThread.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
