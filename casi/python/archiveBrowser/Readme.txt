-*- outline -*-

* Archive Browser
This interactive python archive browser was developed
by Robert Hall, rdh@SLAC.Stanford.EDU.

* Setup
The minimal setup requires python and casi
to interface to the archiver data files.

This means adding <ChannelArchiver>/casi/python
and               <ChannelArchiver>/casi/python/O.$(HOST_ARCH)
to your PYTHONPATH.

Example:
export PYTHONPATH=\
/cs/epics/extensions/src/ChannelArchiver/casi/python/O.Linux:\
/cs/epics/extensions/src/ChannelArchiver/casi/python

If you use a version of python with casi hard-linked in,
you can omit the binary directory O.$(HOST_ARCH).

The file archiveBrowserSetup lists more
environment variables to customize the tool.

The tools calls out to ArchiveManager and ArchiveExport,
so you have to have those.

* USAGE
python archiveBrowser.py [Options] [archive]

Options:
        -channel <channel>             Specify channel name
        -file <configuration file>     Specify configuration file
        -user <user name>              Specify user name (for logging)
        -nolog                         Disables logging
        -help                          Request usage help         

Example: 
	 python archiveBrowser.py ../../../Engine/Test/freq_directory &



