// Tools
#include "ArgParser.h"
// Storage
#include "DirectoryFile.h"
#include "DataFile.h"
#include "DataWriter.h"
#include "DataReader.h"

void header_dump(const stdString &index_name)
{
    DirectoryFile index(index_name);
    DirectoryFileIterator channels = index.findFirst();

    while (channels.isValid())
    {
        printf("Channel '%s':\n", channels.entry.data.name);

        DataFile *datafile =
            DataFile::reference(index.getDirname(),
                                channels.entry.data.first_file, false);
        DataHeader *header =
            datafile->getHeader(channels.entry.data.first_offset);
        datafile->release();
        while (header && header->isValid())
        {
            header->show(stdout);
            header->read_next();
        }
        delete header;
                
        channels.next();
    }
    DataFile::close_all();
}

void add(const stdString &index_name)
{
    DataWriter *writer;

    DirectoryFile *index = new DirectoryFile(index_name, true);
    stdString channel_name = "jane";
    CtrlInfo ctrl_info;
    ctrl_info.setNumeric(3, "Volt",
                         -10.0, 10.0,
                          -9.0,  9.0,
                          -8.0,  7.0);
    DbrType dbr_type = DBR_TIME_DOUBLE;
    DbrCount dbr_count = 1;
    size_t num_samples = 100;

    writer = new DataWriter(*index,
                            channel_name, ctrl_info,
                            dbr_type, dbr_count, 2.0, num_samples);

    dbr_time_double *data =  RawValue::allocate(dbr_type, dbr_count, 1);

    RawValue::setTime(data, epicsTime::getCurrent());
    data->value = 3.14;
    data->status = 0;
    data->severity = 0;
    writer->add(data);
    
    RawValue::setTime(data, epicsTime::getCurrent());
    data->value = 3.15;
    writer->add(data);
    
    RawValue::free(data);
    
    delete writer;
    DataFile::close_all();
    delete index;
}

void value_dump(const stdString &index_name, const stdString &channel_name, epicsTime *start)
{
    DirectoryFile index(index_name);
    DataReader *reader = new DataReader(index);

    const RawValue::Data *data = reader->find(channel_name, start);
    while (data)
    {
        if (start)
        {
            if (RawValue::getTime(data) < *start)
                printf("< ");
            else if (RawValue::getTime(data) > *start)
                printf("> ");
            else
                printf("==");   
        }
        RawValue::show(stdout, reader->dbr_type, reader->dbr_count,
                       data, &reader->ctrl_info);
        data = reader->next();
    }
    delete reader;
}

int main(int argc, const char *argv[])
{ 
    CmdArgParser parser(argc, argv);
    parser.setHeader("Strorage lib. test");
    parser.setArgumentsInfo("<index>");
    CmdArgFlag   dump (parser, "dump",
                       "Dump values");
    CmdArgString start(parser, "start", "<mm/dd/yyyy hh:mm:ss.nnnnnnnn>",
                       "Start time");
    if (parser.parse() == false)
        return -1;
    if (parser.getArguments().size() != 1)
    {
        parser.usage();
        return -1;
    }
    stdString index_name = parser.getArgument(0);

    epicsTime *start_time = 0;

    if (start.isSet())
    {
        start_time = new epicsTime;
        string2epicsTime(start.get(), *start_time);
    }

    //header_dump(*index_name);
    //add(*index_name);

    if (dump)
        value_dump(index_name, "jane", start_time);

    return 0;
}
