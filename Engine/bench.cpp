/*

Machine          hdparm -t     hdparm -T        bench            bench -read
------------------------------------------------------------------------------
500MHz Laptop    12.36 MB/sec  129.29 MB/sec    ~45000 vals/sec  200k vals/sec

hdparm -t /dev/hda: Timing buffered disk reads,
hdparm -T /dev/hda: Timing buffer-cache reads.

Older results with LibIO:
blitz, 800 Mhz NT4.0             20000
ioc94, 266 Mhz RedHat 6.1         7740
gopher1, 500Mhz PIII, RH 6.1     14477
gopher0, 2x333Mhz, RAID5          8600

*/

// Tools
#include <ArgParser.h>
// Storage
#include <DataWriter.h>
#include <DataReader.h>
#include <DataFile.h>

bool write_samples(const stdString &index_name,
                   const stdString &channel_name,
                   size_t samples)
{
    DirectoryFile index(index_name, true);
    CtrlInfo info;
    
    info.setNumeric (2, "socks",
                     0.0, 10.0,
                     0.0, 1.0, 9.0, 10.0);
    DbrType dbr_type = DBR_TIME_DOUBLE;
    DbrCount dbr_count = 1;
    DataWriter * writer =
        new DataWriter(index,
                       channel_name, info,
                       dbr_type, dbr_count, 2.0,
                       samples);
    dbr_time_double *data =  RawValue::allocate(dbr_type, dbr_count, 1);
    data->status = 0;
    data->severity = 0;
    size_t i;
    for (i=0; i<samples; ++i)
    {
        data->value = (double) i;
        RawValue::setTime(data, epicsTime::getCurrent());
        if (!writer->add(data))
        {
            fprintf(stderr, "Write error with value %d/%d\n",
                    i, samples);
            break;
        }   
    }
    RawValue::free(data);
    delete writer;
    DataFile::close_all();
    return true;
}


size_t read_samples(const stdString &index_name,
                    const stdString &channel_name)
{
    DirectoryFile index(index_name, true);
    size_t samples = 0;
    
    DataReader *reader = new DataReader(index);
    const RawValue::Data *data = reader->find(channel_name, 0);

    while (data)
    {
        ++samples;
        data = reader->next();    
    }
    delete reader;
    DataFile::close_all();
    return samples;
}


int main (int argc, const char *argv[])
{
    CmdArgParser parser(argc, argv);
    parser.setHeader("Archive Writing Benchmark");
    parser.setArgumentsInfo("<index>");
    CmdArgInt samples(parser, "samples", "<count>",
                      "Number of samples to write");
    CmdArgString channel_name(parser, "channel", "<name>",
                      "Channel Name");
    CmdArgFlag do_read(parser, "read",
                       "Perform read test");
    
    // defaults
    samples.set(100000);
    channel_name.set("fred");

    if (parser.parse() == false)
        return -1;
    if (parser.getArguments().size() > 1)
    {
        parser.usage();
        return -1;
    }
    stdString index_name = "index";
    if (parser.getArguments().size() == 1)
        index_name = parser.getArgument(0);

    size_t count;
    epicsTime start, stop;
    if (do_read)
    {
        start = epicsTime::getCurrent ();
        count = read_samples(index_name, channel_name.get());
        stop = epicsTime::getCurrent ();
    }
    else
    {
        count = samples.get();
        start = epicsTime::getCurrent ();
        write_samples(index_name, channel_name.get(), count);
        stop = epicsTime::getCurrent ();
    }
    
    double secs = stop - start;
    printf("%d values in %g seconds: %g vals/sec\n",
           count, secs,
           (double)count / secs);
    return 0;
}


