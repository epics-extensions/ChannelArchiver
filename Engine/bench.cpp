// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include <iostream>
#include <BinArchive.h>

/*
Linux hdparm report
-------------------
ioc94, 266 Mhz Linux:
hdparm -t, Timing buffered disk reads: 64 MB in 16.49 sec =  3.88 MB/sec
hdparm -T, Timing buffer-cache reads: 128 MB in  2.58 sec = 49.61 MB/sec

gopher1:
hdparm -t, Timing buffered disk reads: 64 MB in  5.16 sec = 12.40 MB/sec
hdparm -T, Timing buffer-cache reads: 128 MB in  1.00 sec =128.00 MB/sec


Write Test with different LowLevelIO and debug settings
-------------------------------------------------------

Machine                      fd,debug  fd,-O   FILE,debug  FILE,-O
------------------------------------------------------------------
blitz, 800 Mhz NT4.0:          31405 >35000     >20000       20000
ioc94, 266 Mhz RedHat 6.1               300                   7740
gopher1, 500Mhz PIII, RH 6.1:     58     59         57       14477
gopher0, 2x333Mhz, RAID5                 42                   8600

Values per second.
Results vary between test runs,
these tabulated results are not averaged
but just show example results
  
fd: low level file descriptior (open,close,read,write,...)
FILE: stdio (fopen, ...)

*/

const size_t COUNT=100000;

int main ()
{
    BinArchive *archive = new BinArchive ("dir", true /* for write */);
    ChannelIteratorI *channels = archive->newChannelIterator ();
    ValueI *value = archive->newValue (DBR_TIME_DOUBLE, 1);
    ChannelI *channel;

    CtrlInfoI info;

    info.setNumeric (2, "socks",
                     0.0, 10.0,
                     0.0, 1.0, 9.0, 10.0);
    value->setCtrlInfo (&info);
    value->setStatus (0, 0);
    
    stdString name = "fred";
    
    if (! archive->findChannelByName (name, channels))
    {
        archive->addChannel (name, channels);
    }

    channel = channels->getChannel();

    osiTime start = osiTime::getCurrent ();
    

    for (size_t i=0; i<COUNT; ++i)
    {
        value->setDouble ((double) i);
        value->setTime (osiTime::getCurrent());
        
        if (! channel->addValue (*value))
        {
            channel->addBuffer (*value, 10.0, 100);
            if (! channel->addValue (*value))
            {
                std::cout << "Cannot add value\n";
                return -1;
            }
        }
    }
    
    channel->releaseBuffer ();
    osiTime stop = osiTime::getCurrent ();
    double secs = double (stop) - double (start);
    
    std::cout << COUNT << " values in " << secs << " seconds: "
              << COUNT/secs << " vals/sec\n";
    
    delete channels;
    delete value;
    delete archive;

    return 0;
}


