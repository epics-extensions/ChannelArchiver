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
#include <iostream>

inline double fabs(double x)
{ return x>=0 ? x : -x; }

// Loop over current values and find oldest
static osiTime findOldestValue(const stdVector<ValueIteratorI *> &values)
{
    osiTime first;
    for (size_t i=0; i<values.size(); ++i)
    {
        if (! values[i]->isValid())
            continue;
        if (first == nullTime  ||  first > values[i]->getValue()->getTime())
            first = values[i]->getValue()->getTime();
    }

    return first;
}

// Find all values that are stamped 'round' secs
// after 'start' and return the average of those
// time stamps.
static osiTime findRoundedTime(const stdVector<ValueIteratorI *> &values,
                               double round)
{
    const osiTime start = findOldestValue(values);
    double double_start = double(start);
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

Exporter::Exporter(ArchiveI *archive)
{
    init(archive);
}

Exporter::Exporter(ArchiveI *archive, const stdString &filename)
{
    init(archive);
    _filename = filename;
}

void Exporter::init(ArchiveI *archive)
{
    _archive = archive;
    _undefined_value = "#N/A";
    _show_status = false;
    _round_secs = 0.0;
    _linear_interpol_secs = 0.0;
    _gap_factor = 0;
    _fill = false;
    _be_verbose = false;
    _time_col_val_format = false;
    _is_array = false;
    _datacount = 0;
    _max_channel_count = 100;
}

void Exporter::exportMatchingChannels (const stdString &channel_name_pattern)
{
    // Find all channels in archive that match a given pattern.
    // If pattern is empty,
    // all channels will be listed.
    stdVector<stdString> channel_names;

    ChannelIteratorI *channel = _archive->newChannelIterator();
    _archive->findChannelByPattern (channel_name_pattern, channel);
    while (channel->isValid())
    {
        channel_names.push_back (channel->getChannel()->getName ());
        channel->next();
    }
    delete channel;

    exportChannelList (channel_names);
}

void Exporter::printTime(std::ostream *out, const osiTime &time)
{
    int year, month, day, hour, min, sec;
    unsigned long nano;
    osiTime2vals (time, year, month, day, hour, min, sec, nano);

    char buf[80];
    sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d.%09ld",
            month, day, year, hour, min, sec, nano);
    *out << buf;
#if 0
    *out << std::setw(2) << std::setfill('0') << month << '/'
         << std::setw(2) << std::setfill('0') << day   << '/'
         << std::setw(2) << std::setfill('0') << year  << ' '
         << std::setw(2) << std::setfill('0') << hour  << ':'
         << std::setw(2) << std::setfill('0') << min   << ':'
         << std::setw(2) << std::setfill('0') << sec   << '.'
         << std::setw(9) << std::setfill('0') << nano;
#endif
}

void Exporter::printValue(std::ostream *out, const osiTime &time, const ValueI *v)
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

        if (v->getType() == DBR_TIME_STRING  ||
            (info && info->getType() == CtrlInfoI::Enumerated))
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

void Exporter::exportChannelList(const stdVector<stdString> &channel_names)
{
    size_t i, ai, num = channel_names.size();
    stdVector<ChannelIteratorI *> channels(num);
    stdVector<ValueIteratorI *>   base(num);
    stdVector<ValueIteratorI *>   values(num);
    stdVector<ValueI *>           prev_values(num);
    const ValueI *v;
    char info[300];

    _datacount = 0;
    if (channel_names.size() > _max_channel_count)
    {
        sprintf(info,
                "You tried to export %d channels.\n"
                "For performance reason you are limited to export %d channels at once",
                channel_names.size(), _max_channel_count);
        throwDetailedArchiveException(Invalid, info);
        return;
    }

    // Open Channel & ValueIterator
    for (i=0; i<num; ++i)
    {
        channels[i] = _archive->newChannelIterator();
        if (! _archive->findChannelByName(channel_names[i], channels[i]))
        {
            sprintf(info, "Cannot find channel '%s' in archive",
                    channel_names[i].c_str());
            throwDetailedArchiveException (ReadError, info);
            return;
        }
        base[i] = _archive->newValueIterator();
        channels[i]->getChannel()->getValueAfterTime(_start, base[i]);

        if (_linear_interpol_secs > 0.0)
        {
            LinInterpolValueIteratorI *interpol =
                new LinInterpolValueIteratorI(base[i], _linear_interpol_secs);
            interpol->setMaxDeltaT(_linear_interpol_secs * _gap_factor);
            values[i] = interpol;
        }
        else
            values[i] = new ExpandingValueIteratorI(base[i]);

        prev_values[i] = 0;

        if (values[i]->isValid() && values[i]->getValue()->getCount() > 1)
        {
            _is_array = true;
            if (num > 1)
            {
                sprintf(info,
                        "Array channels like '%s' can only be exported on their own, "
                        "not together with another channel",
                        channel_names[i].c_str());
                throwDetailedArchiveException(Invalid, info);
                return;
            }
        }
    }

    std::ostream *out = &std::cout; // default: stdout
    std::ofstream file;
    if (! _filename.empty())
    {
        file.open (_filename.c_str());
#       ifdef __HP_aCC
        if (file.fail ())
#else
        if (! file.is_open ())
#endif
            throwDetailedArchiveException(WriteError, _filename);
        out = &file;
    }

    prolog(*out);

    // Headline: "Time" and channel names
    *out << "Time";
    for (i=0; i<num; ++i)
    {
        *out << '\t' << channels[i]->getChannel()->getName();
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
        time = findOldestValue  (values);

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
        printTime(out, time);
        for (i=0; i<num; ++i)
        {
            if (! values[i]->isValid())
            {   *out << "\t" << _undefined_value;
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
                printValue(out, time, v);
                if (_fill) // keep copy of this value?
                {
                    if (v->isInfo () && prev_values[i])
                    {
                        delete prev_values[i];
                        prev_values[i] = 0;
                    }
                    else if (prev_values[i] && prev_values[i]->hasSameType(*v))
                        prev_values[i]->copyValue(*v);
                    else
                    {
                        delete prev_values[i];
                        prev_values[i] = v->clone();
                    }
                }
                values[i]->next();
            }
            else
            {
                if (prev_values[i])
                    printValue(out, time, prev_values[i]);
                else
                    *out << "\t" << _undefined_value;
            }
        }
        *out << "\n";
        // Find time stamp for next line
        if (_round_secs == 0)
            time = findOldestValue(values);
        else
            time = findRoundedTime(values, _round_secs);
    }

    post_scriptum(channel_names);

    if (out == &file)
        file.close();

    for (i=0; i<num; ++i)
    {
        delete prev_values[i];
        delete values[i];
        delete base[i];
        delete channels[i];
    }
}

