Info
====

This is a python-based tool for recovering a missing or broken index file,
by reconstructing it from data files.
It was contributed by Noboru Yamamoto, (noboru.yamamoto\@kek.jp).

Beginning with version 2-1-1 of the ChannelArchiver toolset, the DATA
and INFO blocks in the binary data files have been marked with
4-ASCII character magic IDs 'DATA' and 'INFO'.
This index recovery tool searches the data files for these markers,
and attempts to create an index file.
The mechanism will likely fail if your PV names include 'DATA' or 'INFO',
or if your PV data itself happens to contain those strings,
either because your string or character-waveform samples really contained
those strings, or because their binary data happened to match those
strings when converted to ASCII.

Prerequisites
=============

* EPICS 3.14.8.2
* MacOSX 10.4.5. Maybe any 10.3.9/10.4.x or Linux?
* Python 2.4.3. Noboru also used 2.4.2.
  The Python 2.3.5 that comes with OSX didn't work.
* SWIG 1.3.29. Noboru also used 1.3.27.
* Channel Archiver 2.9
  Previous releases use a different "Storage" library API
  and won't work.

Compilation
===========
You need to check some constants in setup.py for your EPICS installation.
Then run:
    python2.4 setup.py build
    sudo python2.4 setup.py install

Usage
=====
Run IndexRecoveryMain() in your python interpreter with appropriate parameters:

    cd test
    python2.4
    from IndexRecovery import IndexRecoveryMain
    IndexRecoveryMain("20060331", "new_index")
    <CTRL-D>

The data obtained from the original and the reconstructed index
looks the same:
    for pv in fred jane janet
    do
        echo $pv
        ArchiveExport index $pv | wc
        ArchiveExport new_index $pv | wc
    done

Of course this has only been tested on few platforms and limited data sets.
If you have a damaged index file, that probably means there's some problem
with the data files as well, so the index reconstruction may fail or remain
incomplete.

Again, this was contributed by Noboru, so all the glory is his.

