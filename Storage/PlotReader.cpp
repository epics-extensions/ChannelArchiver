// Base
#include <alarm.h>
// Tools
#include "MsgLogger.h"
// Storage
#include "PlotReader.h"

#undef DEBUG_PLOTREAD

PlotReader::PlotReader(IndexFile &index, double delta)
        : reader(index), delta(delta)
{
    type = 0;
    count = 0;
    type_changed = ctrl_info_changed = false;
    have_initial_final = have_mini_maxi = false;
    initial = final = 0;
    state = s_dunno;
#ifdef DEBUG_PLOTREAD
    printf("Plot Reader, delta %g (%g h)\n", delta, delta/60.0/60.0);
#endif
}

PlotReader::~PlotReader()
{
    RawValue::free(final);
    RawValue::free(initial);
}

const RawValue::Data *PlotReader::find(const stdString &channel_name,
                                       const epicsTime *start)
{
    this->channel_name = channel_name;
    if (!(reader_data = reader.find(channel_name, start)))
        return 0;
    if (delta <= 0.0)
        return reader_data;
    if (start)
    {   // No before-or-at, we want data in the first bin.
        end_of_bin = *start + delta;
        while (RawValue::getTime(reader_data) < *start)
        {
            reader_data = reader.next();
            if (!reader_data)
                return 0;
        }
    }
    else
        end_of_bin = roundTimeUp(RawValue::getTime(reader_data), delta);
    return fill_bin();
}

const RawValue::Data *PlotReader::fill_bin()
{
    double d;
    N = 0;
    have_initial_final = have_mini_maxi = false;
    if (!reader_data)
        return 0;
#ifdef DEBUG_PLOTREAD
    stdString txt;
    printf("End of bin: %s\n", epicsTimeTxt(end_of_bin, txt));
#endif
    while (reader_data   &&   RawValue::getTime(reader_data) < end_of_bin)
    {   // iterate until just after end_of_bin, updating mini, maxi, ...
#ifdef DEBUG_PLOTREAD
        printf("Raw: ");
        RawValue::show(stdout, reader.getType(), reader.getCount(),
                       reader_data, &reader.getInfo());
#endif
        if (reader.changedInfo())
        {
            info = reader.getInfo();
            ctrl_info_changed = true;
        }        
        if (initial == 0 ||
            type != reader.getType() || count != reader.getCount())
        {
            RawValue::free(final);
            RawValue::free(initial);
            type    = reader.getType();
            count   = reader.getCount();
            initial = RawValue::allocate(type, count, 1);
            final   = RawValue::allocate(type, count, 1);
            type_changed = true;
            // If this happens within a bin: Tough - we start over.
            N = 0;
            have_initial_final = have_mini_maxi = false;
            if (!(initial && final))
            {
                LOG_MSG("PlotReader: Cannot allocate data %d/%d\n",
                        type, count);
                return 0;
            }
        }
        RawValue::copy(type, count, final, reader_data);
        ++N;
        if (!have_initial_final)
        {
            RawValue::copy(type, count, initial, reader_data);
            have_initial_final = true;
        }
        if (RawValue::isInfo(final)==false  &&
            RawValue::getDouble(type, count, final, d))
        {
            if (have_mini_maxi)
            {
                if (mini > d)
                    mini = d;
                if (maxi < d)
                    maxi = d;
            }
            else
            {
                mini = maxi = d;
                have_mini_maxi = true;
            }
        }   
        reader_data = reader.next();
    }
    // Options at this point:
    // 1) Found absolutely nothing (!have_initial_final)
    // 2) There is no numeric data (!have_mini_maxi)
    //    and the reader is also done (reader_data == 0)
    // 3) We didn't find any useful data (!have_mini_maxi),
    //    but the reader is still valid:
    //    A gap in time, or raw data that we can't use (string, enum)
    // 4) We found data (have_mini_maxi).
    //    Reader might be valid, or we reached the end of the archive.
    if (have_initial_final)
        state = s_gotit;
    else
        state = s_dunno;  // case 1: Give up
    return next();
}

const RawValue::Data *PlotReader::next()
{
    if (delta <= 0.0)
        return reader.next();
    if (state != s_dunno)
    {   // Anything in current bin?
        LOG_ASSERT(have_initial_final && initial && final);
        double span;
        switch (state)
        {
            case s_gotit:
#ifdef DEBUG_PLOTREAD
                printf("Initial: ");
                RawValue::show(stdout, reader.getType(), reader.getCount(),
                               initial, &reader.getInfo());
#endif
                state = s_ini;
                return initial;
            case s_ini:
                if (N <= 1)
                    break; // only have initial value
                // Try to update initial value to reflect mini/maxi.
                // Need to preserve final value, it's returned later.
                if (N > 2 && have_mini_maxi &&
                    RawValue::setDouble(type, count, initial, mini))
                {   // Artificial time stamp at 1/3 of bin
                    const epicsTime t0 = RawValue::getTime(initial);
                    span = RawValue::getTime(final) - t0;
                    RawValue::setTime(initial, t0 + span/3);
                    RawValue::setStatus(initial, 0, 0);
#ifdef DEBUG_PLOTREAD
                    printf("Minimum: ");
                    RawValue::show(stdout, reader.getType(), reader.getCount(),
                                   initial, &reader.getInfo());
#endif
                    state = s_min;
                    return initial;
                }
                // fall through
            case s_min:
                if (N > 2 && have_mini_maxi &&
                    RawValue::setDouble(type, count, initial, maxi))
                {   // Artificial stamp at 2/3 of bin.
                    // Note t0 is already at 1-3 of original [ini..fin]
                    const epicsTime t0 = RawValue::getTime(initial);
                    span = RawValue::getTime(final) - t0;
                    RawValue::setTime(initial, t0 + span/2);
#ifdef DEBUG_PLOTREAD
                    printf("Maximum: ");
                    RawValue::show(stdout, reader.getType(), reader.getCount(),
                                   initial, &reader.getInfo());
#endif
                    state = s_max;
                    return initial;
                }
                // fall through
            case s_max:
#ifdef DEBUG_PLOTREAD
                printf("Final: ");
                RawValue::show(stdout, reader.getType(), reader.getCount(),
                               final, &reader.getInfo());
#endif
                state = s_fin;
                return final;
            default:
                break;
        }
    }
    // Check next bin
    end_of_bin += delta;
    return fill_bin();
}

DbrType PlotReader::getType() const
{   return (delta <= 0.0) ? reader.getType() : type; }
    
DbrCount PlotReader::getCount() const
{   return (delta <= 0.0) ? reader.getCount() : count; }
    
const CtrlInfo &PlotReader::getInfo() const
{   return (delta <= 0.0) ? reader.getInfo() : info; }
    
bool PlotReader::changedType()
{
    if (delta <= 0.0)
        return reader.changedType();
    bool changed = type_changed;
    type_changed = false;
    return changed;
}

bool PlotReader::changedInfo()
{
    if (delta <= 0.0)
        return reader.changedInfo();
    bool changed = ctrl_info_changed;
    ctrl_info_changed = false;
    return changed;
} 
