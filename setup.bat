@echo off
rem
rem General setup:
rem This should presumably part of your autoexec.bat (Win98)
rem or "System" setup (NT),
rem but can also be adjusted in here and then this batch is
rem called for configuration...

rem Configure:
rem
rem (*) MS Visual Studio

set HOME=c:\Kay

set MSVS=C:\PROGRA~1\VSTUDIO

rem (*) Path: Personal, system, tcl

set PATH=%HOME%\bin
set PATH=%PATH%;c:\WINDOWS;c:\WINDOWS\COMMAND
set PATH=%PATH%;c:\progra~1\tcl\bin

rem (*) EPICS base

set EBASE=%HOME%\Epics\base

rem (*) Python: Add to path,
rem	tell it how to find what we are about to build

set PATH=%PATH%;c:\progra~1\python
set PYTHONPATH=%HOME%\Epics\extensions\src\ChannelArchiver\casi\python\O.WIN32


rem ----------------------------------------------------------
rem Should be ok from here
rem ----------------------------------------------------------

set HOST_ARCH=WIN32

set PATH=%PATH%;%MSVS%\COMMON\MSDEV98\BIN;%MSVS%\VC98\BIN
set PATH=%PATH%;%EBASE%\BIN\%HOST_ARCH%


set INCLUDE=%MSVS%\vc98\INCLUDE
set LIB=%MSVS%\vc98\LIB

