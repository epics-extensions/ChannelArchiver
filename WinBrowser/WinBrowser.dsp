# Microsoft Developer Studio Project File - Name="WinBrowser" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WinBrowser - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WinBrowser.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WinBrowser.mak" CFG="WinBrowser - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WinBrowser - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WinBrowser - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WinBrowser - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "WinBrowser - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "../Export" /I "../LibIO" /I "../../Tools" /I "../../../../base/include" /I "../../../../base/include/os/Win32" /I "../../../../base/src/ca" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D CPP_EDITION=3 /D "OLD_osiTime" /D "_AFXDLL" /D "EPICS_DLL" /D __STDC__=0 /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ca.lib Com.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"../../../../base/lib/Win32"

!ENDIF 

# Begin Target

# Name "WinBrowser - Win32 Release"
# Name "WinBrowser - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ActionEdit.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ArchiveException.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ArchiveI.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\ASCIIParser.cpp
# End Source File
# Begin Source File

SOURCE=.\Axis.cpp
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

SOURCE=.\BrowserDlg.cpp
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

SOURCE=.\DlgExport.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\ExpandingValueIteratorI.cpp
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

SOURCE=..\..\Tools\LowLevelIO.cpp
# End Source File
# Begin Source File

SOURCE=.\MsgBoxF.cpp
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

SOURCE=.\Plot.cpp
# End Source File
# Begin Source File

SOURCE=.\PlotCanvas.cpp
# End Source File
# Begin Source File

SOURCE=..\..\Tools\RegularExpression.cpp
# End Source File
# Begin Source File

SOURCE=..\LibIO\SpreadSheetExporter.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=..\..\Tools\stdString.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeAxis.cpp
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

SOURCE=.\WinBrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\WinBrowser.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ActionEdit.h
# End Source File
# Begin Source File

SOURCE=.\Axis.h
# End Source File
# Begin Source File

SOURCE=.\BrowserDlg.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\DlgExport.h
# End Source File
# Begin Source File

SOURCE=.\DoEvents.h
# End Source File
# Begin Source File

SOURCE=.\Limits.h
# End Source File
# Begin Source File

SOURCE=.\MsgBoxF.h
# End Source File
# Begin Source File

SOURCE=.\Plot.h
# End Source File
# Begin Source File

SOURCE=.\PlotCanvas.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TimeAxis.h
# End Source File
# Begin Source File

SOURCE=.\WinBrowser.h
# End Source File
# Begin Source File

SOURCE=.\XAxis.h
# End Source File
# Begin Source File

SOURCE=.\YAxis.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\WinBrowser.ico
# End Source File
# Begin Source File

SOURCE=.\res\WinBrowser.rc2
# End Source File
# End Group
# End Target
# End Project
