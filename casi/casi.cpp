
#ifdef SWIG
%module casi
#endif

#include <BinArchive.h>
#include <BinChannelIterator.h>
#include <MultiArchive.h>
#include <ArchiveException.h>
#include <ValueI.h>
#include "archive.h"
#include "channel.h"
#include "value.h"
#include "ctrlinfo.h"

// ----------------------------------------------------------------
// o s i T i m e

// Convert into time text format...
//
static const char *epicsTime2txt(const epicsTime &epicsTime)
{
     static char txt[80];
     int year, month, day, hour, min, sec;
     unsigned long nano;

     epicsTime2vals(epicsTime, year, month, day, hour, min, sec, nano);

     sprintf(txt, "%4d/%02d/%02d %02d:%02d:%02d.%09d",
             year, month, day, hour, min, sec, nano);
     return txt;
}

// Convert from time text in "YYYY/MM/DD hh:mm:ss" format
// as used by the archiver with 24h hours and (maybe) fractional seconds
//
static bool text2epicsTime(const char *text, epicsTime &epicsTime)
{
    int     year, month, day, hour, minute;
    double  second;

    if (sscanf(text, "%04d/%02d/%02d %02d:%02d:%lf",
               &year, &month, &day,
               &hour, &minute, &second)  != 6)
        return false;

    int secs = (int)second;
    unsigned long nano = (unsigned long) ((second - secs) * 1000000000L);

    vals2epicsTime(year, month, day, hour, minute, secs, nano, epicsTime);

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

void archive::newValue (int type, int count, value &val)
{
   if (! _archiveI)
      throwDetailedArchiveException(Invalid, "no Archive");
   if (val._valI) delete val._valI;
   val._valI = _archiveI->newValue(type, count);
}


const char *archive::nextFileTime(const char *current_time)
{
    BinArchive *archive = dynamic_cast<BinArchive *>(_archiveI);

    if (! archive)
        throwDetailedArchiveException(Invalid, "no BinArchive");

    epicsTime time;
    if (! text2epicsTime(current_time, time))
        throwDetailedArchiveException(Invalid, "invalid time");

    epicsTime next;
    archive->calcNextFileTime(time, next);

    return epicsTime2txt(next);
}

// ----------------------------------------------------------------
//  c h a n n e l

const size_t channel::init_buf_size = 64;
const size_t channel::max_buf_size = 1024;
const size_t channel::buf_size_fact = 4;

channel::channel()
   : _iter(0), buf_size(init_buf_size)
{
//  cout << "TRACE: channel()\n";
}

channel::~channel()
{
    setIter(0);
//  cout << "TRACE: ~channel()\n";
}

bool channel::valid() const
{
    return _archiveI && _iter && _iter->isValid();
}

const char *channel::name() const
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

const char *channel::getFirstTime() const
{
    if (!valid())
        throwDetailedArchiveException(Invalid,
                                       "getFirstTime called "
                                       "for invalid channel");
    return epicsTime2txt(_iter->getChannel()->getFirstTime());
}

const char *channel::getLastTime() const
{
    if (!valid())
        throwDetailedArchiveException(Invalid,
                                       "getLastTime called "
                                       "for invalid channel");

    return epicsTime2txt(_iter->getChannel()->getLastTime());
}

// internal helper routine
bool channel::testValue(value &v) const
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

bool channel::getFirstValue(value &v) const
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getFirstValue called "
                                       "for invalid channel");
    return _iter->getChannel()->getFirstValue(v._iter);
}

bool channel::getLastValue(value &v) const
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getLastValue called "
                                       "for invalid channel");
    return _iter->getChannel()->getLastValue(v._iter);
}

bool channel::getValueAfterTime(const char *time, value &v) const
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getValueAfterTime called "
                                       "for invalid channel");
        
    epicsTime epicsTime;
    if (! text2epicsTime(time, epicsTime))
    {
        v.setIter(0);
        return false;
    }

    return _iter->getChannel()->getValueAfterTime(epicsTime, v._iter);
}

bool channel::getValueBeforeTime(const char *time, value &v) const
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getValueBeforeTime called "
                                       "for invalid channel");
    epicsTime epicsTime;
    if (! text2epicsTime(time, epicsTime))
    {
        v.setIter (0);
        return false;
    }

    return _iter->getChannel()->getValueBeforeTime(epicsTime, v._iter);
}

bool channel::getValueNearTime(const char *time, value &v) const
{
    if (! testValue(v))
        throwDetailedArchiveException(Invalid,
                                       "getValueNearTime called "
                                       "for invalid channel");
    epicsTime epicsTime;
    if (! text2epicsTime(time, epicsTime))
    {
        v.setIter(0);
        return false;
    }

    return _iter->getChannel()->getValueNearTime(epicsTime, v._iter);
}

int channel::lockBuffer(const value &value)
{
   if (! valid()  ||  !value.valid() )
        return 0;
   
   return _iter->getChannel()->lockBuffer(*value.getVal(), 1);
}

void channel::addBuffer(const value &value, int value_count)
{
   if (! valid ()  || !value.valid() )
      throwDetailedArchiveException(Invalid,
				    "invalid Value");
   if (dynamic_cast<BinChannelIterator *>(_iter) == 0)
      throwDetailedArchiveException(Invalid,
				    "no BinArchive");
   
   _iter->getChannel()->addBuffer(*value.getVal(), 1, value_count);
}

// bool channel::addValue(const value &value)
// {
//     if (! valid()  ||  !value.valid())
//         throwDetailedArchiveException(Invalid,
//                                        "invalid Value");
//     return _iter->getChannel()->addValue(*value.getVal());
// }

bool channel::addValue(const value &value)
{
    if (! valid()  ||  !value.valid())
        throwDetailedArchiveException(Invalid,
				      "invalid Value");
    if (_iter->getChannel()->lockBuffer(*value.getVal(), 1) == 0) {
       _iter->getChannel()->addBuffer(*value.getVal(), 1, buf_size);
       if (buf_size < max_buf_size)
	  buf_size *= buf_size_fact;
    }
    
    return _iter->getChannel()->addValue(*value.getVal());
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
   : _iter(0), _valI(0)
{
//  cout << "TRACE: value()\n";
}

value::~value()
{
    setIter (0);
//  cout << "TRACE: ~value()\n";
}

bool value::valid() const
{
    return _valI || (_iter && _iter->isValid());
}

const ValueI *value::getVal() const {
   return _valI?_valI:_iter->getValue();
}

bool value::isInfo() const
{
    if (! valid())
        throwDetailedArchiveException (Invalid,
                                       "invalid Value");
    return getVal()->isInfo();
}

const char *value::type() const
{
    static char txt[20];
    
    if (! valid())
        return "<invalid>";

    DbrType type = getVal()->getType();
    if (type < dbr_text_dim)
        return dbr_text[type];
    
    sprintf (txt, "unknown: 0x%X", type);
    return txt;
}

int value::ntype() const
{
    if (! valid())
        return -1;

    return getVal()->getType();
}

int value::count() const
{
    if (! valid())
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
    return getVal()->getCount();
}

double value::getidx(int index) const
{
    if (! valid())
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
    return getVal()->getDouble(index);
}

void value::setidx(double v, int index)
{
   if (!_valI)
      throwDetailedArchiveException(Invalid,
				    "can't set read only value!");
   _valI->setDouble(v, index);
}

void value::parse( const char *str )
{
   if (!_valI)
      throwDetailedArchiveException(Invalid,
				    "can't set read only value!");
   _valI->parseValue( str );
}

const char *value::text() const
{
    static size_t l = 0; // Text has to be kept after return
    static char *text = 0;
   
    stdString value;
    if (valid())
    {
        getVal()->getValue(value);
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

const char *value::time() const
{
    if (! valid())
        throwDetailedArchiveException (Invalid,
                                       "invalid Value");
    return epicsTime2txt (getVal()->getTime());
}

double value::getDoubleTime() const
{
    if (! valid())
        throwDetailedArchiveException (Invalid,
                                       "invalid Value");
    struct timeval tv = getVal()->getTime();
    return (double)tv.tv_sec + tv.tv_usec/1e6;
}

bool value::isRepeat() const
{
   if (!valid())
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
   return (( sevr() == ARCH_REPEAT ) || ( sevr() == ARCH_EST_REPEAT));
}


const char *value::status() const
{
    static stdString text; // keep after 'return'
    if (!valid())
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
    getVal()->getStatus(text);
    return text.c_str();
}

int value::stat() const
{
    if (!valid())
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
    return getVal()->getStat();
}

int value::sevr() const
{
    if (!valid())
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
    return getVal()->getSevr();
}


void value::setStat(int stat, int sevr)
{
   if (!_valI)
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
   _valI->setStatus(stat, sevr);
}

void value::setStatus( const char* stat)
{
   if (!_valI)
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
   _valI->parseStatus(stat);
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
    epicsTime time;
    if (! text2epicsTime(until, time))
        return 0;
        
    return _iter->determineChunk (time);
}

void value::setIter(ValueIteratorI *iter)
{
    if (_iter)
        delete _iter;
    _iter = iter;
}

void value::getCtrlInfo(ctrlinfo &ci) const
{
   ci.setValue(getVal());
}

void value::setCtrlInfo(ctrlinfo &ci)
{
   if (!_valI || !ci._ctrli)
        throwDetailedArchiveException(Invalid,
                                      "invalid Value");
   _valI->setCtrlInfo(ci._ctrli);
}

void value::setTime (const char *ts)
{
   if (!_valI)
      throwDetailedArchiveException(Invalid, "invalid Value");

   epicsTime epicsTime;
   if (!text2epicsTime(ts, epicsTime))
      throwDetailedArchiveException(Invalid, "invalid Time");
   _valI->setTime(epicsTime);
}

void value::clone(value& src)
{
   _valI = src.getVal()->clone();
}


// ----------------------------------------------------------------
//  c o n t r o l - i n f o
ctrlinfo::ctrlinfo()
   : _ctrli(0), _enumCnt(0), _enumStr(0)
{  
   _ctrli = new CtrlInfoI();
}

ctrlinfo::~ctrlinfo()
{
   if (_ctrli) delete _ctrli;
}

long ctrlinfo::getPrecision() const 
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return _ctrli->getPrecision();
}

const char *ctrlinfo::getUnits() const
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return _ctrli->getUnits();
}

float ctrlinfo::getDisplayHigh() const
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return _ctrli->getDisplayHigh();
}

float ctrlinfo::getDisplayLow() const
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return _ctrli->getDisplayLow();
}

float ctrlinfo::getHighAlarm() const
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return _ctrli->getHighAlarm();
}

float ctrlinfo::getHighWarning() const
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return _ctrli->getHighWarning();
}

float ctrlinfo::getLowWarning() const
{
   if (!_ctrli) 
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return _ctrli->getLowWarning();
}

float ctrlinfo::getLowAlarm() const
{
   if (!_ctrli) 
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return _ctrli->getLowAlarm();
}

void ctrlinfo::setValue(const ValueI* v)
{
   if (!v) 
      throwDetailedArchiveException(Invalid,
				    "invalid Value");
   if (_ctrli) delete _ctrli;
   _ctrli = new CtrlInfoI(*(v->getCtrlInfo()));
}

Type ctrlinfo::getType() const
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return (Type)_ctrli->getType();
}

void ctrlinfo::setNumeric(long prec, const char *units,
		float disp_low, float disp_high,
		float low_alarm, float low_warn, 
		float high_warn, float high_alarm)
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   _ctrli->setNumeric(prec, units, disp_low, disp_high, 
		      low_alarm, low_warn, high_warn, high_alarm);
}

void ctrlinfo::setEnumeratedString(int state, const char *str)
{
   if (state != _enumCnt)
      throwDetailedArchiveException(Invalid,
				    "EnumeratedString out of sequence");
   if (state)
      _enumStr = (char**)realloc(_enumStr, state * sizeof(char*));
   else
      _enumStr = (char**)malloc(state * sizeof(char*));
   if (!_enumStr)
      throwDetailedArchiveException(Invalid,
				    "can't allocate string");
   _enumStr[state] = strdup(str);
   _enumCnt++;
}


void ctrlinfo::setEnumerated()
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   size_t sumlen = 0;
   for (size_t i = 0; i < _enumCnt; i++)
      sumlen += strlen(_enumStr[i]) + 1;
   _ctrli->allocEnumerated(_enumCnt, sumlen);
   for (size_t i = 0; i < _enumCnt; i++) {
      _ctrli->setEnumeratedString(i, _enumStr[i]);
      free(_enumStr[i]);
   }
   free(_enumStr);
   _enumCnt = 0;
}

int ctrlinfo::getNumStates() const
{
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   return _ctrli->getNumStates();
}

const char* ctrlinfo::getState(int state) const
{
   static char *stateStr = 0;
   
   if (!_ctrli)
      throwDetailedArchiveException(Invalid,
				    "invalid ControlInfo");
   stdString str;
   _ctrli->getState(state, str);
   if (stateStr) free(stateStr);
   stateStr = strdup(str.c_str());
   return stateStr;
}
