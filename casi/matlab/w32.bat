@echo off
rem W32.BAT
rem
rem Compile and link options used for building ChannelArchiver MEX-files
rem using the Microsoft Visual C++ compiler version 6.0 
rem Based on the mexopts.bat file created via "mex -setup".
rem
rem kasemir@lanl.gov

set HOST_ARCH=WIN32

rem EPICS extensions (where the ChannelArchiver includes/libs are)
set EXTENSIONS=../../../..

rem EPICS base
set BASE=s:/cs/epics/R3.13.3/base

rem Visual C++
set MSVCDir=C:\Program Files\VStudio\VC98

set MATLAB=%MATLAB%
set MSDevDir=%MSVCDir%\..\Common\msdev98
set PATH=%MSVCDir%\BIN;%MSDevDir%\bin;%PATH%
set INCLUDE=%EXTENSIONS%/include;%BASE%/include
set INCLUDE=%MSVCDir%\INCLUDE;%MSVCDir%\MFC\INCLUDE;%MSVCDir%\ATL\INCLUDE;%INCLUDE%
set LIB=%EXTENSIONS%/lib/%HOST_ARCH%;%BASE%/lib/%HOST_ARCH%
set LIB=%MSVCDir%\LIB;%MSVCDir%\MFC\LIB;%LIB%

rem Compiler parameters

rem /Zi     : debug info
rem /W3
rem -GR -GX : RTTI, exceptions
rem /MDd    : link with MSVCRTD.LIB debug lib

set COMPILER=cl
set COMPFLAGS=-c -DWIN32 -DEPICS_DLL_NO -Zp8 -W3 -DMATLAB_MEX_FILE -nologo -GR -GX -Zi /MDd
set OPTIMFLAGS=-O2 -Oy- -DNDEBUG
set NAME_OBJECT=/Fo

rem Linker parameters

set LIBLOC=%MATLAB%\extern\lib\win32\microsoft\msvc60
set LINKER=link
set LINKFLAGS=/dll /export:%ENTRYPOINT% /MAP /LIBPATH:"%LIBLOC%" libmx.lib libmex.lib libmatlbmx.lib libmat.lib /implib:%LIB_NAME%.x
set LINKOPTIMFLAGS=
set LINKDEBUGFLAGS=/debug

rem Added the ChannelArchiver and EPICS base libs, wsock32

set LINK_FILE=ChanArchIOObj.lib ToolsObj.lib ca.lib Com.lib wsock32.lib
set LINK_LIB=
set NAME_OUTPUT=/out:"%OUTDIR%%MEX_NAME%.dll"
set RSP_FILE_INDICATOR=@

rem Resource compiler parameters

set RC_COMPILER=rc /fo "%OUTDIR%mexversion.res"
set RC_LINKER=

set POSTLINK_CMDS=del "%OUTDIR%%MEX_NAME%.map"
set POSTLINK_CMDS1=del %LIB_NAME%.x

