# Microsoft Developer Studio Generated NMAKE File, Based on atac.dsp
!IF "$(CFG)" == ""
CFG=atac - Win32 Debug
!MESSAGE No configuration specified. Defaulting to atac - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "atac - Win32 Release" && "$(CFG)" != "atac - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "atac.mak" CFG="atac - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "atac - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "atac - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "atac - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\atac.dll"


CLEAN :
	-@erase "$(INTDIR)\atac.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\atac.dll"
	-@erase "$(OUTDIR)\atac.exp"
	-@erase "$(OUTDIR)\atac.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ATAC_EXPORTS" /Fp"$(INTDIR)\atac.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\atac.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\atac.pdb" /machine:I386 /out:"$(OUTDIR)\atac.dll" /implib:"$(OUTDIR)\atac.lib" 
LINK32_OBJS= \
	"$(INTDIR)\atac.obj"

"$(OUTDIR)\atac.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "atac - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\atac.dll"


CLEAN :
	-@erase "$(INTDIR)\atac.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\atac.dll"
	-@erase "$(OUTDIR)\atac.exp"
	-@erase "$(OUTDIR)\atac.ilk"
	-@erase "$(OUTDIR)\atac.lib"
	-@erase "$(OUTDIR)\atac.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I "..\..\tcl\tcl8.0.3\generic" /I "..\..\..\include" /I "..\..\..\..\base\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "ATAC_EXPORTS" /Fp"$(INTDIR)\atac.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\atac.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib tcl80.lib ChanArchIOObj.lib ToolsObj.lib ws2_32.lib ca.lib Com.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\atac.pdb" /debug /machine:I386 /out:"$(OUTDIR)\atac.dll" /implib:"$(OUTDIR)\atac.lib" /pdbtype:sept /libpath:"..\..\tcl\tcl8.0.3\win\release" /libpath:"..\..\..\lib\win32" /libpath:"..\..\..\..\base\lib\win32" 
LINK32_OBJS= \
	"$(INTDIR)\atac.obj"

"$(OUTDIR)\atac.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("atac.dep")
!INCLUDE "atac.dep"
!ELSE 
!MESSAGE Warning: cannot find "atac.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "atac - Win32 Release" || "$(CFG)" == "atac - Win32 Debug"
SOURCE=.\atac.cpp

"$(INTDIR)\atac.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

