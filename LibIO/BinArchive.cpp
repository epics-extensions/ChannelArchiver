#include "ArchiveException.h"
#include "BinArchive.h"
#include "BinChannelIterator.h"
#include "BinValueIterator.h"
#include "DirectoryFile.h"
#include "Filename.h"

BinArchive::BinArchive(const stdString &archive_name, bool for_write)
{
    initOsiHelpers();
    _dir = new DirectoryFile(archive_name, for_write);
    _secs_per_file = SECS_PER_MONTH;
}

BinArchive::~BinArchive()
{
    delete _dir;
    _dir = 0;
}

ChannelIteratorI *BinArchive::newChannelIterator() const
{
    return new BinChannelIterator();
}

ValueIteratorI *BinArchive::newValueIterator() const
{
    return new BinValueIterator();
}

ValueI *BinArchive::newValue(DbrType type, DbrCount count)
{
    return BinValue::create(type, count);
}

bool BinArchive::findFirstChannel (ChannelIteratorI *channel)
{
    BinChannelIterator *c = dynamic_cast<BinChannelIterator*>(channel);
    c->attach (this, _dir->findFirst ());
    return c->isValid();
}

bool BinArchive::findChannelByName (const stdString &name, ChannelIteratorI *channel)
{
    BinChannelIterator *c = dynamic_cast<BinChannelIterator*>(channel);
    c->attach (this, _dir->find (name));
    return c->isValid();
}

bool BinArchive::findChannelByPattern (const stdString &regular_expression,
                                       ChannelIteratorI *channel)
{
    if (regular_expression.empty ())
        return findFirstChannel (channel);

    BinChannelIterator *c = dynamic_cast<BinChannelIterator*>(channel);
    c->attach (this, _dir->findFirst(), regular_expression.c_str());
    return c->isValid ();
}

bool BinArchive::addChannel (const stdString &name, ChannelIteratorI *channel)
{
    BinChannelIterator *c = dynamic_cast<BinChannelIterator*>(channel);
    c->attach (this, _dir->add (name));
    return c->isValid();
}

void BinArchive::makeDataFileName (const ValueI &value, stdString &data_file_name)
{
    int year, month, day, hour, min, sec;
    unsigned long nano;
    char buffer[80];

    osiTime time = value.getTime();
    osiTime file;

    if (getSecsPerFile() == SECS_PER_MONTH)
    {
        osiTime2vals (time, year, month, day, hour, min, sec, nano);
        vals2osiTime (year, month, 1, 0, 0, 0, 0, file);
    }
    else
        file = roundTimeDown (time, getSecsPerFile());
    osiTime2vals (file, year, month, day, hour, min, sec, nano);
    sprintf (buffer, "%04d%02d%02d-%02d%02d%02d", year, month, day, hour, min, sec);
    data_file_name = buffer;
}

void BinArchive::calcNextFileTime (const osiTime &time, osiTime &next_file_time)
{
    if (!isValidTime (time))
    {
        LOG_MSG ("BinArchive::calcNextFileTime: given invalid time\n");
        next_file_time = nullTime;
        return;
    }
    if (getSecsPerFile() == SECS_PER_MONTH)
    {
        int year, month, day, hour, min, sec;
        unsigned long nano;
        osiTime2vals (time, year, month, day, hour, min, sec, nano);
        if (month > 11)
        {
            month = 1;
            ++year;
        }
        else
            ++month;
        vals2osiTime (year, month, 1, 0, 0, 0, 0, next_file_time);
    }
    else
    {
        osiTime file = roundTimeDown (time, getSecsPerFile());

        // compute the next time a file will need to be created
        file += 1; // in case we are right on midnight
        next_file_time = roundTimeUp (file, getSecsPerFile());
    }
}

bool BinArchive::makeFullFileName (const stdString &basename, stdString &full_name)
{
    if (! isValidFilename (basename))
        return false;
    LOG_ASSERT (_dir);
    Filename::build (_dir->getDirname(), basename, full_name);
    return true;
}


