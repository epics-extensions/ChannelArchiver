// Tools
#include "ArgParser.h"
// Storage
#include "DirectoryFile.h"
#include "DataFile.h"
#include "OldDataWriter.h"
#include "DataWriter.h"
#include "OldDataReader.h"
#include "DataReader.h"
#include "SpreadsheetReader.h"

bool verbose;

void old_list_channels(const stdString &index_name)
{
    DirectoryFile dir;
    if (!dir.open(index_name))
    {
        fprintf(stderr, "Cannot open dir. file %s\n",
                index_name.c_str());
        return;
    }
    DirectoryFileIterator dfi = dir.findFirst();
    while (dfi.isValid())
    {
        printf("%s\n", dfi.entry.data.name);
        dfi.next();
    }
}

void list_channels(const stdString &index_name)
{
    archiver_Index index;
    if (!index.open(index_name.c_str()))
    {
        fprintf(stderr, "Cannot open index %s\n",
                index_name.c_str());
        return;
    }
    channel_Name_Iterator *cni = index.getChannelNameIterator();
    if (!cni)
    {
        fprintf(stderr, "Cannot get channel name iterator\n");
        return;
    }
    stdString name;
    bool ok = cni->getFirst(&name);
    while (ok)
    {
        printf("%s\n", name.c_str());
        ok = cni->getNext(&name);
    }
    delete cni;
}

void header_dump(const stdString &index_name)
{
    DirectoryFile index;
    if (! index.open(index_name))
    {
        fprintf(stderr, "Cannot open index %s\n",
                index_name.c_str());
	return;
    }
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

void old_add(const stdString &index_name)
{
    OldDataWriter *writer;

    DirectoryFile *index = new DirectoryFile();
    if (!index && index->open(index_name, true))
    {
        fprintf(stderr, "Cannot open index %s\n", index_name.c_str());
        return;
    }
    stdString channel_name = "jane";
    CtrlInfo ctrl_info;
    ctrl_info.setNumeric(3, "Volt",
                         -10.0, 10.0,
                          -9.0,  9.0,
                          -8.0,  7.0);
    DbrType dbr_type = DBR_TIME_DOUBLE;
    DbrCount dbr_count = 1;
    size_t num_samples = 100;

    writer = new OldDataWriter(*index,
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

void add(const stdString &index_name, const stdString &channel_name, int count)
{
    DataWriter *writer;

    archiver_Index *index = new archiver_Index();
    if (!index && index->open(index_name.c_str(), false))
    {
        fprintf(stderr, "Cannot open index %s\n", index_name.c_str());
        return;
    }
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
    while (count > 0)
    {
        RawValue::setTime(data, epicsTime::getCurrent());
        data->value = 3.14 + count;
        data->status = 0;
        data->severity = 0;
        writer->add(data);
        --count;
    } 
    RawValue::free(data);
    
    delete writer;
    DataFile::close_all();
    delete index;
}

void old_value_dump(const stdString &index_name,
                    const stdString &channel_name,
                    epicsTime *start, epicsTime *end)
{
    DirectoryFile index;
    if (!index.open(index_name))
    {
        fprintf(stderr, "Cannot open index %s\n", index_name.c_str());
        return;
    }
    OldDataReader *reader = new OldDataReader(index);

    const RawValue::Data *data = reader->find(channel_name, start);
    while (data  &&   (!end  ||  RawValue::getTime(data) < *end))
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

void value_dump(const stdString &index_name,
                const stdString &channel_name,
                epicsTime *start, epicsTime *end)
{
    archiver_Index index;
    if (!index.open(index_name.c_str()))
    {
        fprintf(stderr, "Cannot open index %s\n", index_name.c_str());
        return;
    }
    DataReader *reader = new DataReader(index);

    const RawValue::Data *data = reader->find(channel_name, start, end);
    while (data  &&   (!end  ||  RawValue::getTime(data) < *end))
    {
        if (start && verbose)
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

void run_test(const stdString &index_name)
{
    archiver_Index index;
    if (!index.open(index_name.c_str()))
    {
        fprintf(stderr, "Cannot open index %s\n", index_name.c_str());
        return;
    }
    SpreadsheetReader sheet(index);
    stdVector<stdString> names;
    names.push_back("jane");
    names.push_back("janet");
    names.push_back("freddy");
    sheet.find(names, 0);


}   

int main(int argc, const char *argv[])
{ 
    CmdArgParser parser(argc, argv);
    parser.setHeader("Storage lib. test\n");
    parser.setArgumentsInfo("<index>");
    CmdArgFlag   verb_flag(parser, "verbose", "Be verbose");
    CmdArgString channel(parser, "channel", "<name>", "Channel name");
    CmdArgFlag   list_chans(parser, "list", "List Channel names");
    CmdArgFlag   headers(parser, "headers", "Dump header information");
    CmdArgFlag   dump   (parser, "dump", "Dump values");
    CmdArgString start  (parser, "start", "<mm/dd/yyyy hh:mm:ss.nnnnnnnn>",
                         "Start time");
    CmdArgString end    (parser, "end", "<mm/dd/yyyy hh:mm:ss.nnnnnnnn>",
                         "End time");
    CmdArgInt add_values(parser, "add", "<count>", "Add values");
    CmdArgFlag   old    (parser, "old", "Use old directory file routines");
    CmdArgFlag   test   (parser, "test", "Run some test code");
    
    channel.set("jane");
    if (parser.parse() == false)
        return -1;
    if (parser.getArguments().size() != 1)
    {
        parser.usage();
        return -1;
    }
    verbose = verb_flag;
    stdString index_name = parser.getArgument(0);
    epicsTime *start_time = 0, *end_time = 0;
    if (start.isSet())
    {
        start_time = new epicsTime;
        string2epicsTime(start.get(), *start_time);
    }
    if (end.isSet())
    {
        end_time = new epicsTime;
        string2epicsTime(end.get(), *end_time);
    }
    if (list_chans)
    {
        if (old)
            old_list_channels(index_name);
        else
            list_channels(index_name);
    }
    if (headers)
        header_dump(index_name);
    if (add_values.get() > 0)
        add(index_name, channel, add_values.get());
    if (dump)
    {
        if (old)
            old_value_dump(index_name, channel, start_time, end_time);
        else
            value_dump(index_name, channel, start_time, end_time);
    }
    if (test)
        run_test(index_name);

    DataFile::close_all(true);
        
    return 0;
}
