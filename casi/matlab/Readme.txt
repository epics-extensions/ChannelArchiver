-*- outline -*-

* Info
This is a Matlab extension (MEX). It's C/C++ code that provides
a Matlab binding to the ChannelArchiver LibIO.
The Matlab manual ("External Interfaces" in Matlab 6) contains
detailed examples on how to configure Matlab and how to compile
MEX extensions. In here I can only provide an example for doing
this with the Win32 version of Matlab 6.

* Usage
Run Matlab:
>>cd 's:\cs\epics\extensions\src\ChannelArchiver\casi\Matlab'
>>helpwin archive

* Compilation
** Matlab preparations
Start Matlab. In there:
>> mex -setup
I picked Microsoft Visual C/C++ version 6.0 in C:\Program Files\VStudio 

This created a file which we will have to adjust soon:
"c:\Documents and Settings\Kay\Application Data\Mathworks\MATLAB\R12\mexopts.bat"

Test 101: mex basically works if you can build the explore example.
>>cd 'c:/sys/matlabR12/extern/examples/mex'
>>mex explore.c
>>explore(2)

Now the hard part: That mex configuration does not know about the
ChannelArchiver includes and libraries.

I copied the mexopts.bat into ChannelArchiver/casi/matlab/w32.bat
and performed some modifications which are commented in w32.bat.
Hopefully you can adjust the first few site specifics to get going.
For Linux or other OS, you are on your own.

* Building
In Matlab:
>>cd 's:\cs\epics\extensions\src\ChannelArchiver\casi\Matlab'
>>makew32

The makew32.m Matlab file compiles each MEX file like this:
>>mex -f w32.bat archive.cpp
>>mex -f w32.bat archive_close.cpp
...

In my case, the 's:' drive is actually served by a Linux box via
Samba. Matlab didn't see the newly created DLLs until I
changed directory to someplace else and then back to casi/matlab.


