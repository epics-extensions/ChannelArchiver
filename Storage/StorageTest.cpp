
#include "DirectoryFile.h"
#include "DataFile.h"
#include "DataWriter.h"

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

int main(int argc, const char *argv[])
{
    stdString index_name = argv[1];

    //header_dump(index_name);

    add(index_name);

    return 0;
}
