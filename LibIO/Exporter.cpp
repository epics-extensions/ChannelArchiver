// $Id$
//
// Exporter.cpp
//
// Base class for tools that export data from a ChannelArchive
//

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "Exporter.h"
#include "ExpandingValueIteratorI.h"
#include "LinInterpolValueIteratorI.h"
#include "ArchiveException.h"
#include <fstream>

BEGIN_NAMESPACE_CHANARCH
using namespace std;

inline double fabs (double x)
{ return x>=0 ? x : -x; }

// Loop over current values and find oldest
static osiTime findOldestValue	(const vector<ValueIteratorI *> &values)
{
	osiTime first;
	for (size_t i=0; i<values.size(); ++i)
	{
		if (! values[i]->isValid())
			continue;
		if (first == nullTime  ||  first > values[i]->getValue()->getTime ())
			first = values[i]->getValue()->getTime ();
	}

	return first;
}

// Find all values that are stamped 'round' secs
// after 'start' and return the average of those
// time stamps.
static osiTime findRoundedTime	(const vector<ValueIteratorI *> &values, double round)
{
	const osiTime start = findOldestValue (values);
	double double_start = double (start);
	double double_time, middle = 0.0;
	size_t i, num_in_range = 0;

	for (i=0; i<values.size(); ++i)
	{
		if (values[i]->isValid())
		{
			double_time = double(values[i]->getValue()->getTime());
			if (fabs (double_start - double_time) <= round)
			{
				middle += double_time;
				++num_in_range;
			}
		}
	}

	return num_in_range > 1 ? osiTime (middle / num_in_range) : start;
}

Exporter::Exporter (ArchiveI *archive, const stdString &filename)
:	_archive (archive)
{
	_filename = filename;
	_undefined_value = "#N/A";
	_show_status = false;
	_round_secs = 0.0;
	_linear_interpol_secs = 0.0;
	_fill = false;
	_be_verbose = false;
	_time_col_val_format = false;
	_is_array = false;
	_datacount = 0;
	_max_channel_count = 10;
}

void Exporter::exportMatchingChannels (const stdString &channel_name_pattern)
{
	// Find all channels in archive that match a given pattern.
	// If pattern is empty,
	// all channels will be listed.
	vector<stdString> channel_names;

	ChannelIteratorI *channel = _archive->newChannelIterator();
	_archive->findChannelByPattern (channel_name_pattern, channel);
	while (channel->isValid())
	{
		if (_be_verbose)
			clog << "# Exporting channel '" << channel->getChannel()->getName () << "'\n";
		channel_names.push_back (channel->getChannel()->getName ());
		channel->next();
	}
	delete channel;

	exportChannelList (channel_names);
}

void Exporter::printTime (ostream *out, const osiTime &time)
{
	int year, month, day, hour, min, sec;
	unsigned long nano;
	osiTime2vals (time, year, month, day, hour, min, sec, nano);

	*out << month << '/' << day << '/' << year << ' '
		<< setw(2) << setfill('0') << hour << ':'
		<< setw(2) << setfill('0') << min << ':'
		<< setw(2) << setfill('0') << sec << '.'
		<< setw(9) << setfill('0') << nano;
}

void Exporter::printValue (ostream *out, const osiTime &time, const ValueI *v)
{
	// skip values which are Archiver specials
	size_t ai;
	stdString txt;
	if (v->isInfo ())
	{
		for (ai=0; ai<v->getCount(); ++ai)
			*out << '\t' << _undefined_value;
	}
	else
	{
		const CtrlInfoI *info = v->getCtrlInfo ();

		if (info && info->getType() == CtrlInfoI::Enumerated)
		{
			v->getValue (txt);
			*out << '\t' << txt;
		}
		else
		{
			if (_is_array && _time_col_val_format)
			{
				*out << '\t' << 0 << '\t' << v->getDouble (0) << "\n";
				for (ai=1; ai<v->getCount(); ++ai)
				{
					printTime (out, time);
					*out << '\t' << ai << '\t' << v->getDouble (ai) << "\n";
				}
			}
			else
				for (ai=0; ai<v->getCount(); ++ai)
					*out << '\t' << v->getDouble (ai);
		}
	}

	if (_show_status)
	{
		v->getStatus (txt);
		if (txt.empty ())
			*out << "\t ";
		else
			*out << '\t' << txt;
	}
}

void Exporter::exportChannelList (const vector<stdString> &channel_names)
{
	size_t i, ai, num = channel_names.size();
	vector<ChannelIteratorI *> channels (num);
	vector<ValueIteratorI *> base (num);
	vector<ValueIteratorI *> values (num);
	vector<ValueI *> prev_values (num);
	const ValueI *v;

	_datacount = 0;
	if (channel_names.size() > _max_channel_count)
	{
		strstream info;
		info << "You tried to export " << channel_names.size() << " channels.\n"
			"For performance reason you are limited to export "
			<< _max_channel_count << " channels at once" << '\0';
		throwDetailedArchiveException (Invalid, info.str());
		info.freeze (false); // well, never reached ...
		return;
	}

	// Open Channel & ValueIterator
	for (i=0; i<num; ++i)
	{
		channels[i] = _archive->newChannelIterator ();
		if (! _archive->findChannelByName (channel_names[i], channels[i]))
		{
			strstream info;
			info << "Cannot find channel '" << channel_names[i] << "' in archive" << '\0';
			throwDetailedArchiveException (ReadError, info.str());
			info.freeze (false); // not reached ...
			return;
		}
		base[i] = _archive->newValueIterator ();
		channels[i]->getChannel()->getValueAfterTime (_start, base[i]);

		if (_linear_interpol_secs > 0.0)
			values[i] = new LinInterpolValueIteratorI (base[i], _linear_interpol_secs);
		else
			values[i] = new ExpandingValueIteratorI (base[i]);

		prev_values[i] = 0;

		if (values[i]->isValid() && values[i]->getValue()->getCount() > 1)
		{
			_is_array = true;
			if (num > 1)
			{
				strstream info;
				info << "Array channels like '" << channel_names[i]
					<< "' can only be exported on their own, not together with another channel"
					<< '\0';
				throwDetailedArchiveException (Invalid, info.str());
				info.freeze (false);
				return;
			}
		}
	}

	ostream *out = &cout; // default: stdout
	ofstream file;
	if (! _filename.empty())
	{
		file.open (_filename.c_str());
		if (! file.is_open ())
			throwDetailedArchiveException (WriteError, _filename);
		out = &file;
	}

	prolog (*out);

	// Headline: "Time" and channel names
	*out << "Time";
	for (i=0; i<num; ++i)
	{
		*out << '\t' << channels[i]->getChannel()->getName ();
		if (! values[i]->isValid())
			continue;
		if (values[i]->getValue()->getCtrlInfo()->getType() == CtrlInfoI::Numeric)
			*out << " [" << values[i]->getValue()->getCtrlInfo()->getUnits() << ']';
		// Array columns
		for (ai=1; ai<values[i]->getValue()->getCount(); ++ai)
			*out << "\t[" << ai << ']';
		if (_show_status)
			*out << "\t" << "Status";
	}
	*out << "\n";

	// Find first time stamp
	osiTime time;
	if (_round_secs > 0)
		time = findRoundedTime (values, _round_secs);
	else
		time = findOldestValue	(values);

	bool same_time;
	while (time != nullTime && (_end==nullTime  ||  time <= _end))
	{
#if 0
		// Debug	
		for (i=0; i<num; ++i)
		{
			if (values[i]->isValid())
				*out << i << ": " << *values[i]->getValue() << "\n";
			else
				*out << i << ": " << _undefined_value << "\n";
		}
		*out << "->" << time << "\n";
#endif

		
		
		++_datacount;
		// One line: time and all values for that time
		printTime (out, time);
		for (i=0; i<num; ++i)
		{
			if (! values[i]->isValid())
			{	*out << "\t" << _undefined_value;
				continue;
			}
			v = values[i]->getValue();

			// print all values that match the current time stamp:
			if (_round_secs > 0)
				same_time = fabs(double(v->getTime()) - double(time)) <= _round_secs;
			else
				same_time = v->getTime() == time;
			if (same_time)
			{
				printValue (out, time, v);
				if (_fill) // keep copy of this value?
				{
					if (v->isInfo () && prev_values[i])
					{
						delete prev_values[i];
						prev_values[i] = 0;
					}
					else if (prev_values[i] && prev_values[i]->hasSameType (*v))
						prev_values[i]->copyValue (*v);
					else
					{
						delete prev_values[i];
						prev_values[i] = v->clone ();
					}
				}
				values[i]->next();
			}
			else
			{
				if (prev_values[i])
					printValue (out, time, prev_values[i]);
				else
					*out << "\t" << _undefined_value;
			}
		}
		*out << "\n";
		// Find time stamp for next line
		if (_round_secs == 0)
			time = findOldestValue	(values);
		else
			time = findRoundedTime (values, _round_secs);
	}

	post_scriptum (channel_names);

	if (file.is_open ())
		file.close ();

	for (i=0; i<num; ++i)
	{
		delete prev_values[i];
		delete values[i];
		delete base[i];
		delete channels[i];
	}
}

END_NAMESPACE_CHANARCH
