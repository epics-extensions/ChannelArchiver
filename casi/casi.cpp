
#ifdef SWIG
%module casi
#endif

#include <BinArchive.h>
#include <BinChannelIterator.h>
#include <MultiArchive.h>
#include <ArchiveException.h>
#include "archive.h"
#include "channel.h"
#include "value.h"

// ----------------------------------------------------------------
// o s i T i m e

// Convert into time text format...
//
static const char *osi2txt(const osiTime &osi)
{
     static char txt[80];
     int year, month, day, hour, min, sec;
     unsigned long nano;

     osiTime2vals(osi, year, month, day, hour, min, sec, nano);

     sprintf(txt, "%4d/%02d/%02d %02d:%02d:%02d.%09d",
             year, month, day, hour, min, sec, nano);
     return txt;
}

// Convert from time text in "YYYY/MM/DD hh:mm:ss" format
// as used by the archiver with 24h hours and (maybe) fractional seconds
//
static bool text2osi(const char *text, osiTime &osi)
{
    int     year, month, day, hour, minute;
    double  second;

    if (sscanf(text, "%04d/%02d/%02d %02d:%02d:%lf",
               &year, &month, &day,
               &hour, &minute, &second)  != 6)
        return false;

    int secs = (int)second;
    unsigned long nano = (unsigned long) ((second - secs) * 1000000000L);

    vals2osiTime(year, month, day, hour, minute, secs, nano, osi);

    return true;
}

// ----------------------------------------------------------------
//  a r c h i v e

archive::archive()
{
    _name = "<undefined>";
    _archiveI = 0;
}

archive::~archive()
{
//  cout << "TRACE: ~archive(" << _name << ")\n";
    if (_archiveI)
        delete _archiveI;
}

bool archive::open(const char *name)
{
    _name = name;

    if (_archiveI)
        delete _archiveI;

    try
    {
        _archiveI = new MultiArchive(_name);
    }
    catch (ArchiveException &e)
    {
        _name = "<error: cannot open>";
        return false;
    }

    return true;
}

bool archive::findChannelByName(const char *name, channel &c)
{
    if (! _archiveI)
        return false;
    
    if (c._iter == 0)
        c.setIter(_archiveI);

    return _archiveI->findChannelByName(name, c._iter);
}

bool archive::findChannelByPattern(const char *pattern, channel &c)
{
    if (! _archiveI)
        return false;
    
    if (c._iter == 0)
        c.setIter (_archiveI);

    return _archiveI->findChannelByPattern (pattern, c._iter);
}

bool archive::findFirstChannel(channel &c)
{
    if (! _archiveI)
        return false;
    
    if (c._iter == 0)
        c.setIter(_archiveI);

    return _archiveI->findFirstChannel(c._iter);
}

const char *archive::name()
{
    BinArchive *bin = dynamic_cast<BinArchive *>(_archiveI);
    MultiArchive *multi = dynamic_cast<MultiArchive *>(_archiveI);

    stdString info;
    info = _name;
    if (bin)
        info += " (BinArchive)";
    if (multi)
        info += " (MultiArchive)";
    
    return info.c_str();
}

bool archive::write(const char *name, double hours_per_file)
{
    if (_archiveI)
        delete _archiveI;
    
    try
    {
        _name = name;
        BinArchive *arch = new BinArchive(name, true /* for write */);
        arch->setSecsPerFile((unsigned long) (hours_per_file * 60 * 60));
        _archiveI = arch;
    }
    catch (ArchiveException &e)
    {
        _name = "<error: cannot create/append>";
        return false;
    }

    return true;
}

bool archive::addChannel(const char *name, channel &c)
{
    if (! _archiveI)
        return false;

    if (c._iter == 0)
        c.setIter(_archiveI);
    else // check for BinArchive compliance
    {
        if (dynamic_cast<BinChannelIterator *>(c._iter) == 0)
            throwDetailedArchiveException(Invalid,
                                          "no BinArchive");
    }
        
    return _archiveI->addChannel(name, c._iter);
}

const char *archive::nextFileTime(const char *current_time)
{
    BinArchive *archive = dynamic_cast<BinArchive *>(_archiveI);

    if (! archive)
        throwDetailedArchiveException(Invalid, "no BinArchive");

    osiTime time;
    if (! text2osi(current_time, time))
        throwDetailedArchiveException(Invalid, "invalid time");

    osiTime next;
    archive->calcNextFileTime(time, next);

    return osi2txt(next);
}

// ----------------------------------------------------------------
//  c h a n n e l

channel::channel()
{
    _iter = 0;
//  cout << "TRACE: channel()\n";
}

channel::~channel()
{
    setIter(0);
//  cout << "TRACE: ~channel()\n";
}

bool channel::valid()
{
    return _archiveI && _iter && _iter->isValid();
}

const char *channel::name()
{
    if (valid())
        return _iter->getChannel()->getName();

    return "<invalid>";
}

bool channel::next()
{
    if (_iter)
        return _iter->next();

    return false;
}

const char *channel::getFirstTime()
{
    if (!valid())
        throwDetailedArchiveException(Invalid,
                                       "getFirstTime called "
                                       "for invalid channel");
    return osi2txt(_iter->getChannel()->getFirstTime());
}

const char *channel::getLastTime()
{
    if (!valid())
        throwDetailedArchiveException(Invalid,
                                       "getLastTime called "
                                       "for invalid channel");

    return osi2txt(_iter->getChannel()->getLastTime());
}

// internal helper routine
bool channel::testValue(value &v)
{
    if (!valid())
    {
        v.setIter(0);
        return false;
    }

    if (v._iter == 0)
        v.setIter(_archiveI->newValueIterator());
    return true;
}

bool channel::getFirstValue(value &v)
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getFirstValue called "
                                       "for invalid channel");
    return _iter->getChannel()->getFirstValue(v._iter);
}

bool channel::getLastValue(value &v)
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getLastValue called "
                                       "for invalid channel");
    return _iter->getChannel()->getLastValue(v._iter);
}

bool channel::getValueAfterTime(const char *time, value &v)
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getValueAfterTime called "
                                       "for invalid channel");
        
    osiTime osi;
    if (! text2osi(time, osi))
    {
        v.setIter(0);
        return false;
    }

    return _iter->getChannel()->getValueAfterTime(osi, v._iter);
}

bool channel::getValueBeforeTime(const char *time, value &v)
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getValueBeforeTime called "
                                       "for invalid channel");
    osiTime osi;
    if (! text2osi(time, osi))
    {
        v.setIter (0);
        return false;
    }

    return _iter->getChannel()->getValueBeforeTime(osi, v._iter);
}

bool channel::getValueNearTime(const char *time, value &v)
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getValueNearTime called "
                                       "for invalid channel");
    osiTime osi;
    if (! text2osi(time, osi))
    {
        v.setIter(0);
        return false;
    }

    return _iter->getChannel()->getValueNearTime(osi, v._iter);
}

int channel::lockBuffer(const value &value)
{
    if (! valid()  ||  value._iter == 0)
        return 0;
    
    return _iter->getChannel()->lockBuffer(*value._iter->getValue(),
                                            value._iter->getPeriod());
}

void channel::addBuffer(const value &value, int value_count)
{
    if (! valid ()  ||  value._iter == 0)
        throwDetailedArchiveException(Invalid,
                                       "invalid Value");
    if (dynamic_cast<BinChannelIterator *>(_iter) == 0)
        throwDetailedArchiveException(Invalid,
                                       "no BinArchive");

    _iter->getChannel()->addBuffer(*value._iter->getValue(),
                                    value._iter->getPeriod(),
                                    value_count);
}

bool channel::addValue(const value &value)
{
    if (! valid()  ||  value._iter == 0)
        throwDetailedArchiveException(Invalid,
                                       "invalid Value");

    return _iter->getChannel()->addValue(*value._iter->getValue());
}

void channel::releaseBuffer()
{
    if (! valid())
        return;
    
    _iter->getChannel()->releaseBuffer();
}

void channel::setIter(ArchiveI *archiveI)
{
    if (_iter)
        delete _iter;

    _archiveI = archiveI;
    if (_archiveI)
        _iter = _archiveI->newChannelIterator();
    else
        _iter = 0;
}

// ----------------------------------------------------------------
//  v a l u e

value::value()
{
    _iter = 0;
//  cout << "TRACE: value()\n";
}

value::~value()
{
    setIter (0);
//  cout << "TRACE: ~value()\n";
}

bool value::valid()
{
    return _iter && _iter->isValid();
}

bool value::isInfo()
{
    if (! valid())
        throwDetailedArchiveException (Invalid,
                                       "invalid Value");
    return _iter->getValue()->isInfo();
}

const char *value::type()
{
    static char txt[20];
    
    if (! valid())
        return "<invalid>";

    DbrType type = _iter->getValue()->getType();
    if (type < dbr_text_dim)
        return dbr_text[type];
    
    sprintf (txt, "unknown: 0x%X", type);
    return txt;
}

int value::count()
{
    if (! valid())
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
    return _iter->getValue()->getCount();
}

double value::get()
{
    return getidx(0);
}

double value::getidx(int index)
{
    if (! valid())
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
    return _iter->getValue()->getDouble(index);
}

const char *value::text()
{
    static size_t l = 0; // Text has to be kept after return
    static char *text = 0;
   
    stdString value;
    if (valid())
    {
        _iter->getValue()->getValue(value);
        if (l < value.length())
        {
            if (text)
                free(text);
            text = (char *) malloc(value.length() + 1);
            l = value.length() + 1;
        }
        memcpy(text, value.c_str(), value.length()+1);
        return text;
    }

    return "<invalid>";
}

const char *value::time()
{
    if (! valid())
        throwDetailedArchiveException (Invalid,
                                       "invalid Value");
    return osi2txt (_iter->getValue()->getTime());
}

double value::getDoubleTime()
{
    osiTime cur_time;
    if (! valid())
        throwDetailedArchiveException (Invalid,
                                       "invalid Value");
    cur_time = _iter->getValue()->getTime();
    return ((double)cur_time);

}

const char *value::status()
{
    static stdString text; // keep after 'return'
    if (!valid())
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
    _iter->getValue()->getStatus(text);
    return text.c_str();
}

bool value::next()
{
    if (_iter)
        return _iter->next();
    return false;
}

bool value::prev()
{
    if (_iter)
        return _iter->prev();
    return false;
}

int value::determineChunk(const char *until)
{
    if (!_iter)
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
    osiTime time;
    if (! text2osi(until, time))
        return 0;
        
    return _iter->determineChunk (time);
}

void value::setIter(ValueIteratorI *iter)
{
    if (_iter)
        delete _iter;
    _iter = iter;
}

