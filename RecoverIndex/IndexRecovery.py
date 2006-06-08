#!/usr/local/bin/python

from IndexFile import *
from ChanArchDataFile import *
import sets

def IndexRecoveryMain(datafile="20060331",indexfilename="recovered_index"):
    print "handling :", datafile
    index=IndexFile(50)
    index.open(stdString(indexfilename),0)
    data_blocks,cntrinfo_blocks=read_data_file(datafile)
    data_blocks.reverse()
    data_filename=stdString(datafile)
    next_data_files=sets.Set()
    print "** Adding data to index : ", indexfilename
    for data_block in data_blocks:
        name=stdString(data_block.name)
        ts=timespec()
        ts.tv_sec=data_block.begin_time_sec+int(EPICS_EPOCH_UTC)
        ts.tv_nsec=data_block.begin_time_nsec
        starttime=epicsTime(ts)
        ts=timespec()
        ts.tv_sec=data_block.end_time_sec+int(EPICS_EPOCH_UTC)
        ts.tv_nsec=data_block.end_time_nsec
        endtime=epicsTime(ts)
        directory=stdString()
        offset=data_block.offset
        print "Block: '%s', %s @ %d" % (data_block.name, datafile, offset)
        rtreep=index.addChannel(name,directory)
        rtreep.insertDatablock(starttime, endtime, offset, data_filename)
        if data_block.next_file and data_block.next_file != datafile:
            next_data_files.add(data_block.next_file)
    index.close()
    for data_file in next_data_files:
        print "Next File:",data_file
        IndexRecoveryMain(data_file,indexfilename)
        
        
