
#ifdef SWIG
%module example
#endif

#include <BinArchive.h>
#include <ArchiveException.h>
USE_STD_NAMESPACE
USING_NAMESPACE_CHANARCH
#include "archive.h"
#include "channel.h"
#include "value.h"

// ----------------------------------------------------------------
// o s i T i m e

// Convert into time text format...
//
static const char *osi2txt (const osiTime &osi)
{
	static char txt[80];
	int year, month, day, hour, min, sec;
	unsigned long nano;

	osiTime2vals (osi, year, month, day, hour, min, sec, nano);

	sprintf (txt, "%4d/%02d/%02d %02d:%02d:%02d.%09d",
		year, month, day, hour, min, sec, nano);
	return txt;
}

// Convert from time text in "YYYY/MM/DD hh:mm:ss" format
// as used by the archiver with 24h hours and (maybe) fractional seconds
//
static bool text2osi (const char *text, osiTime &osi)
{
	int	year, month, day, hour, minute;
	double second;

	if (sscanf (text, "%04d/%02d/%02d %02d:%02d:%lf",
		&year, &month, &day,
		&hour, &minute, &second)  != 6)
		return false;

	int secs = (int) second;
	unsigned long nano = (unsigned long) ((second - secs) * 1000000000L);

	vals2osiTime (year, month, day, hour, minute, secs, nano, osi);

	return true;
}

// ----------------------------------------------------------------
//  a r c h i v e

archive::archive ()
{
	_name = "<undefined>";
	_archiveI = 0;
}

archive::~archive ()
{
	cout << "TRACE: ~archive(" << _name << ")\n";
	if (_archiveI)
		delete _archiveI;
}

bool archive::open (const char *name)
{
	_name = name;

	if (_archiveI)
		delete _archiveI;

	try
	{
		_archiveI = new BinArchive (_name);
	}
	catch (ArchiveException &e)
	{
		_name = "<error>";
		return false;
	}

	return true;
}

bool archive::findChannelByName (const char *name, channel &c)
{
	if (c._iter == 0)
		c.setIter (_archiveI);

	return _archiveI->findChannelByName (name, c._iter);
}

bool archive::findChannelByPattern (const char *pattern, channel &c)
{
	if (c._iter == 0)
		c.setIter (_archiveI);

	return _archiveI->findChannelByPattern (pattern, c._iter);
}

bool archive::findFirstChannel (channel &c)
{
	if (c._iter == 0)
		c.setIter (_archiveI);

	return _archiveI->findFirstChannel (c._iter);
}

// ----------------------------------------------------------------
//  c h a n n e l

channel::channel ()
{
	_iter = 0;
	cout << "TRACE: channel()\n";
}

channel::~channel ()
{
	setIter (0);
	cout << "TRACE: ~channel()\n";
}

bool channel::valid ()
{
	return _archiveI && _iter && _iter->isValid();
}

const char *channel::name ()
{
	if (valid())
		return _iter->getChannel()->getName();

	return "<invalid>";
}

bool channel::next ()
{
	if (_iter)
		return _iter->next ();

	return false;
}

const char *channel::getFirstTime ()
{
	if (!valid())
		return "<invalid>";

	return osi2txt (_iter->getChannel()->getFirstTime());
}

const char *channel::getLastTime ()
{
	if (!valid())
		return "<invalid>";

	return osi2txt (_iter->getChannel()->getLastTime());
}

bool channel::testValue (value &v)
{
	if (!valid())
	{
		v.setIter (0);
		return false;
	}

	if (v._iter == 0)
		v.setIter (_archiveI->newValueIterator());
	return true;
}

bool channel::getFirstValue (value &v)
{
	if (! testValue (v))
		return false;

	return _iter->getChannel()->getFirstValue (v._iter);
}

bool channel::getLastValue (value &v)
{
	if (! testValue (v))
		return false;

	return _iter->getChannel()->getLastValue (v._iter);
}

bool channel::getValueAfterTime (const char *time, value &v)
{
	if (! testValue (v))
		return false;
	osiTime osi;
	if (! text2osi (time, osi))
	{
		v.setIter (0);
		return false;
	}

	return _iter->getChannel()->getValueAfterTime (osi, v._iter);
}

bool channel::getValueBeforeTime (const char *time, value &v)
{
	if (! testValue (v))
		return false;
	osiTime osi;
	if (! text2osi (time, osi))
	{
		v.setIter (0);
		return false;
	}

	return _iter->getChannel()->getValueBeforeTime (osi, v._iter);
}

bool channel::getValueNearTime (const char *time, value &v)
{
	if (! testValue (v))
		return false;
	osiTime osi;
	if (! text2osi (time, osi))
	{
		v.setIter (0);
		return false;
	}

	return _iter->getChannel()->getValueNearTime (osi, v._iter);
}

void channel::setIter (ArchiveI *archiveI)
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

value::value ()
{
	_iter = 0;
	cout << "TRACE: value()\n";
}

value::~value ()
{
	setIter (0);
	cout << "TRACE: ~value()\n";
}

bool value::valid ()
{
	return _iter && _iter->isValid();
}

bool value::isInfo ()
{
	if (! valid())
		return true; // invalid: some type of info...
	return _iter->getValue()->isInfo();
}

const char *value::type ()
{
	static char txt[20];

	if (! valid())
		return "<invalid>";

	DbrType type = _iter->getValue()->getType ();
	if (type < dbr_text_dim)
		return dbr_text[type];

	sprintf (txt, "unknown: 0x%X", type);
	return txt;
}

int value::count ()
{
	if (! valid())
		return 0;

	return _iter->getValue()->getCount();
}

double value::get ()
{
	return getidx (0);
}

double value::getidx (int index)
{
	if (! valid())
		return 0.0;

	return _iter->getValue()->getDouble(index);
}

const char *value::text ()
{
	static stdString value_text; // keep after 'return'
	if (valid())
	{
		_iter->getValue()->getValue(value_text);
		return value_text.c_str();
	}

	return "<invalid>";
}

const char *value::time ()
{
	if (valid())
	{
		return osi2txt (_iter->getValue()->getTime());
	}

	return "<invalid>";
}

const char *value::status ()
{
	static stdString text; // keep after 'return'
	if (valid())
	{
		_iter->getValue()->getStatus(text);
		return text.c_str();
	}

	return "<invalid>";
}

bool value::next ()
{
	if (_iter)
		return _iter->next ();

	return false;
}

bool value::prev ()
{
	if (_iter)
		return _iter->prev ();

	return false;
}

void value::setIter (ValueIteratorI *iter)
{
	if (_iter)
		delete _iter;
	_iter = iter;
}

