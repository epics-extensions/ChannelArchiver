
#include "DirectoryFile.h"
#include "DataFile.h"

int main(int argc, const char *argv[])
{
    stdString index_name = argv[1];

    DirectoryFile index(index_name);
    DirectoryFileIterator channels = index.findFirst();

    while (channels.isValid())
    {
        printf("Channel '%s':\n", channels.entry.data.name);

        DataFile *datafile =
            DataFile::reference(index.getDirname(),
                                channels.entry.data.first_file, false);
        DataHeaderIterator *header =
            datafile->getHeader(channels.entry.data.first_offset);
        while (header && header->isValid())
        {
            printf("Header '%s' @ 0x%lX\n",
                   datafile->getBasename().c_str(),
                   header->getOffset());
            header->getNext();
        }
        delete header;
        datafile->release();
                
        channels.next();
    }
    DataFile::close_all();

    return 0;
}
