// SpreadSheetExporter.cpp

#include "SpreadSheetExporter.h"
#include "ArchiveException.h"
#include "LinInterpolValueIteratorI.h"
#include "ExpandingValueIteratorI.h"
#include <fstream>
#include <iostream>

SpreadSheetExporter::SpreadSheetExporter(ArchiveI *archive)
        : Exporter(archive)
{
    _use_matlab_format = false;
}

SpreadSheetExporter::SpreadSheetExporter(ArchiveI *archive,
                                         const stdString &filename)
        : Exporter(archive, filename)
{
    _use_matlab_format = false;
}

inline double fabs(double x)
{ return x>=0 ? x : -x; }

// Loop over current values and find oldest
static osiTime findOldestValue(ValueIteratorI *values[], size_t num)
{
    size_t i;
    osiTime first, t;
    for (i=0; i<num; ++i) // get first valid time
    {
        if (values[i]->isValid())
        {
            first = values[i]->getValue()->getTime();
            break;
        }
    }
    for (++i; i<num; ++i) // see if anything is older
    {
        if (values[i]->isValid())
        {
            t = values[i]->getValue()->getTime();
            if (first > t)
                first = t;
        }
    }

    return first;
}

void SpreadSheetExporter::exportChannelList(
    const stdVector<stdString> &channel_names)
{
    size_t i, ai, num = channel_names.size();
    ChannelIteratorI **channels = 0;
    ValueIteratorI   **base = 0;
    ValueIteratorI   **values = 0;
    ValueI           **prev_values = 0;
    CtrlInfoI        **infos = 0;
    const ValueI *v;
    char info[300];

    if (_use_matlab_format)
    {
        _undefined_value = "NaN";
        _comment = "% ";
    }
    else
    {
        _undefined_value = "#N/A";
        _comment = "; ";
    }
    
    if (num > _max_channel_count)
    {
        sprintf(info,
                "You tried to export %d channels.\n"
                "For performance reason you are limited "
                "to export %d channels at once",
                num, _max_channel_count);
        throwDetailedArchiveException(Invalid, info);
        return;
    }

    channels = new ChannelIteratorI *[num];
    base = new ValueIteratorI *[num];
    values = new ValueIteratorI *[num];
    prev_values = new ValueI *[num];
    infos = new CtrlInfoI *[num];
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
        prev_values[i] = 0;
        infos[i] = new CtrlInfoI();
        values[i] = 0;

        if (! channels[i]->getChannel()->getValueBeforeTime(_start, base[i]))
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

        if (values[i]->isValid() && values[i]->getValue()->getCount() > 1)
        {
            _is_array = true;
            if (num > 1)
            {
                sprintf(info,
                        "Array channels like '%s' can only be exported "
                        "on their own, "
                        "not together with another channel",
                        channel_names[i].c_str());
                throwDetailedArchiveException(Invalid, info);
            }
        }
    }

    std::ostream *out = &std::cout; // default: stdout
    std::ofstream file;
    if (! _filename.empty())
    {
        file.open (_filename.c_str());
#       if defined(HP_UX)
        if (file.fail ())
#else
        if (! file.is_open ())
#endif
        {
            throwDetailedArchiveException(WriteError, _filename);
        }
        out = &file;
    }

    if (_use_matlab_format)
    {
        *out << "% Generated from archive data\n";
        *out << "%\n";
        *out << "% To plot this data into MatLab:\n";
        *out << "%\n";
        *out << "% [date, time, ch1, ch2]=textread('data.txt', '%s %s %f %f', 'commentstyle', 'matlab');\n";
        *out << "%\n";
        *out << "% (replace ch1, ch2 by the names you like, check for a matching number of %f format arguments)\n";
        *out << "%\n";
        *out << "% You will obtain arrays date, time, ch1, ....\n";
        *out << "% To plot one of them over time:\n";
        *out << "% times=datenum(date) + datenum(time);\n";
        *out << "% plot(times, ch1); datetick('x');\n";
        *out << "\n";
    }
    else
    {
        *out << "; Generated from archive data\n";
        *out << ";\n";
        *out << "; To plot this data in MS Excel:\n";
        *out << "; 1) choose a suitable format for the Time colunm (1st col.)\n";
        *out << ";    Excel can display up to the millisecond level\n";
        *out << ";    if you choose a custom cell format like\n";
        *out << ";              mm/dd/yy hh:mm:ss.000\n";
        *out << "; 2) Select the table and generate a plot,\n";
        *out << ";    a useful type is the 'XY (Scatter) plot'.\n";
        *out << ";\n";
        *out << "; '#N/A' is used to indicate that there is no valid data for\n";
        *out << "; this point in time because e.g. the channel was disconnected.\n";
        *out << "; A seperate 'status' column - if included - shows details.\n";
        *out << ";\n";
    }

    if (_linear_interpol_secs > 0.0)
	{
		*out << _comment << "NOTE:\n";
		*out << _comment << "The values in this table\n";
		*out << _comment << "were interpolated within " << _linear_interpol_secs << " seconds\n";
		*out << _comment << "(linear interpolation), so that the values for different channels\n";
		*out << _comment << "can be written on lines for the same time stamp.\n";
		*out << _comment << "If you prefer to look at the exact time stamps for each value\n";
		*out << _comment << "export the data without interpolation.\n";
	}
	else if (_fill)
	{
		*out << _comment << "NOTE:\n";
		*out << _comment << "The values in this table were filled, i.e. repeated\n";
		*out << _comment << "using staircase interpolation.\n";
		*out << _comment << "If you prefer to look at the exact time stamps\n";
		*out << _comment << "for all channels, export the data without rounding.\n";
	}
	else
	{
		*out << _comment << "This table holds the raw data as found in the archive.\n";
		*out << _comment << "Since Channels are not always scanned at exactly the same time,\n";
		*out << _comment << "many '" << _undefined_value
             << "' may appear when looking at more than one channel like this.\n";
	}

    // Headline: "Time" and channel names
    if (_use_matlab_format)
        *out << _comment << "Date / Time";
    else
        *out << "Time";
    for (i=0; i<num; ++i)
    {
        *out << '\t' << channels[i]->getChannel()->getName();
        if (! values[i]->isValid())
            continue;
        if (values[i]->getValue()->getCtrlInfo()->getType()
            == CtrlInfoI::Numeric)
            *out << " [" << values[i]->getValue()->getCtrlInfo()->getUnits()
                 << ']';
        // Array columns
        for (ai=1; ai<values[i]->getValue()->getCount(); ++ai)
            *out << "\t[" << ai << ']';
        if (_show_status)
            *out << "\t" << "Status";
    }
    *out << "\n";

    // Find first time stamp
    osiTime time = findOldestValue(values, num);
    while (time != nullTime)
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
        // One line: time and all values for that time
        printTime(out, time);
        for (i=0; i<num; ++i)
        {
            // print all valid values that match the current time stamp:
            if (values[i]->isValid()  &&
                (v=values[i]->getValue())->getTime() == time)
            {
                ++_data_count;
                printValue(out, time, v);
                if (_fill) // keep copy of this value?
                {
                    if (v->isInfo () && prev_values[i])
                    {
                        delete prev_values[i];
                        prev_values[i] = 0;
                    }
                    /* would be faster but leads to crashes:
                     * Have to figure out when to copy the CtrlInfo and when
                     * the pointer can be kept. Pointer changes when we switch
                     * data files or even cross archive boundaries
                    else if (prev_values[i] && prev_values[i]->hasSameType(*v))
                        prev_values[i]->copyValue(*v);
                        */
                    else
                    {
                        delete prev_values[i];
                        prev_values[i] = v->clone();
                        *infos[i] = *v->getCtrlInfo();
                        prev_values[i]->setCtrlInfo(infos[i]);
                    }
                }
                values[i]->next();
            }
            else
            {   // no valid value for current time stamp:
                if (prev_values[i])
                    printValue(out, time, prev_values[i]);
                else
                    *out << "\t" << _undefined_value;
            }
        }
        *out << "\n";
        // Print one value beyond time, then quit:
        if (isValidTime(_end) && time >= _end)
            break;
        // Find time stamp for next line
        time = findOldestValue(values, num);
    }

    if (out == &file)
        file.close();

    for (i=0; i<num; ++i)
    {
        delete infos[i];
        delete prev_values[i];
        delete values[i];
        delete base[i];
        delete channels[i];
    }
    delete [] infos;
    delete [] prev_values;
    delete [] values;
    delete [] base;
    delete [] channels;
}








