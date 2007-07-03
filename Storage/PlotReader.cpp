// Base
#include <alarm.h>
// Tools
#include "MsgLogger.h"
// Storage
#include "PlotReader.h"

// #define DEBUG_PLOTREAD

PlotReader::PlotReader(Index &index, double delta)
  : delta(delta),
    reader(index),
    reader_data(0),
    type(0),
    count(0),
    type_changed(false),
    have_initial(false),
    have_mini(false),
    have_maxi(false),
    have_info(false),
    have_final(false),
    ctrl_info_changed(false),
    sample_index(0),
    current(0)
{
#ifdef DEBUG_PLOTREAD
    printf("Plot Reader, delta %g (%g h)\n", delta, delta/60.0/60.0);
#endif
}

const RawValue::Data *PlotReader::find(const stdString &channel_name,
                                       const epicsTime *start)
{
    reader_data = reader.find(channel_name, start);
    if (!reader_data)
        return 0;
    if (delta <= 0.0)
        return reader_data;
    if (start)
        end_of_bin = *start + delta;
    else
        end_of_bin = roundTimeUp(RawValue::getTime(reader_data), delta);
    return analyzeBin();
}

const stdString &PlotReader::getName() const
{
    return reader.getName();
}                                  

void PlotReader::clearValues()
{
    have_initial = false;
    have_mini = false;
    have_maxi = false;
    have_info = false;
    have_final = false;
    for (int i=0; i<BinSampleCount; ++i)
        samples[i] = 0;
    sample_index = 0;
    current = 0;
}

const RawValue::Data *PlotReader::analyzeBin()
{
    clearValues();   
    if (!reader_data) // done
        return 0;
    if (RawValue::getTime(reader_data) > end_of_bin)
    {   // Continue where the data is, skip bins that have nothing anyway
        end_of_bin = roundTimeUp(RawValue::getTime(reader_data), delta);
    }
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
        // Check for changing info
        if (reader.changedInfo())
        {
            info = reader.getInfo();
            ctrl_info_changed = true;
        }        
        // Check for changing type type, count...
        if (!have_initial ||
            type != reader.getType() || count != reader.getCount())
        {
            type    = reader.getType();
            count   = reader.getCount();
            initial_sample = RawValue::allocate(type, count, 1);
            mini_sample    = RawValue::allocate(type, count, 1);
            maxi_sample    = RawValue::allocate(type, count, 1);
            info_sample    = RawValue::allocate(type, count, 1);
            final_sample   = RawValue::allocate(type, count, 1);
            type_changed = true;
            // If this happens within a bin: Tough - we start over.
            clearValues();   
        }
        // Get the initial sample
        if (! have_initial)
        {
            RawValue::copy(type, count, initial_sample, reader_data);
            have_initial = true;
        }
        // Hit a special value (disconnected, off)?
        // Leave that in 'final', jump to end of bin and and break.
        if (RawValue::isInfo(reader_data))
        {
            RawValue::copy(type, count, info_sample, reader_data);
            have_info = true;
        }
        else
        {   // non-info sample...   
            double dbl;
            if (RawValue::getDouble(type, count, reader_data, dbl))
            {   // Determine the minimum and maximum.
                if (have_mini == false  ||  dbl < mini_dbl)
                {
                    mini_dbl = dbl;
                    RawValue::copy(type, count, mini_sample, reader_data);
                    have_mini = true;
                }
                if (have_maxi == false  ||  dbl > maxi_dbl)
                {
                    maxi_dbl = dbl;
                    RawValue::copy(type, count, maxi_sample, reader_data);
                    have_maxi = true;
                }
            }
        }   
        // So far, that's also the 'final', since we don't know, yet,
        // if there will be another sample to follow.
        RawValue::copy(type, count, final_sample, reader_data);
        have_final = true;
        reader_data = reader.next();
    }
    
    // Analyzed bin
#ifdef DEBUG_PLOTREAD
        printf("Analyzed bin:\n");
        if (have_initial)
        {
            printf("init  = ");
            RawValue::show(stdout, type, count, initial_sample, &info);
        }
        if (have_mini)
        {
            printf("mini  = ");
            RawValue::show(stdout, type, count, mini_sample, &info);
        }
        if (have_maxi)
        {
            printf("maxi  = ");
            RawValue::show(stdout, type, count, maxi_sample, &info);
        }
        if (have_info)
        {
            printf("info  = ");
            RawValue::show(stdout, type, count, info_sample, &info);
        }
        if (have_final)
        {
            printf("final = ");
            RawValue::show(stdout, type, count, final_sample, &info);
        }
#endif

    // Collect the key samples that we found,
    // inserting them in time order, but avoid duplicates
    if (have_initial)
        addSample(initial_sample);
    if (have_mini)
        addSample(mini_sample);
    if (have_maxi)
        addSample(maxi_sample);
    if (have_info)
        addSample(info_sample);
    if (have_final)
        addSample(final_sample);
    
    // Prepare next bin
    end_of_bin += delta;
    
    return next();
}

/** Add sample to _samples_, inserting in time order, but ignoring dups. */
void PlotReader::addSample(const RawValue::Data *sample)
{
    epicsTime new_time = RawValue::getTime(sample);
    for (int i=0; i<BinSampleCount; ++i)
    {
        if (samples[i] == 0)
        {   // Found empty slot (at end), insert, done.
            samples[i] = sample;
            return;
        }
        // Is this a duplicate sample?
        // (Example: The 'minimum' could be the same as the 'initial'.
        //           Or if there was only one sample, initial == final)
        if (RawValue::equal(type, count, samples[i], sample))
            return; // don't add duplicate
        // Compare times: Need to insert before?
        epicsTime cur_time = RawValue::getTime(samples[i]);
        if (new_time < cur_time)
        {   // Scoot everything down, make room for new sample
            for (int l=BinSampleCount-1; l>i; --l)
                samples[l] = samples[l-1];     
            // Insert new sample, done.
            samples[i] = sample;
            return;
        }
        // Else: Check next slot
    }
}

const RawValue::Data *PlotReader::next()
{
    if (delta <= 0.0) // no binning at all
    {
        current = reader.next();
        return current;
    }
    // Is there another sample in the current bin?
    if (sample_index < BinSampleCount)
    {
        current = samples[sample_index];
        ++sample_index;
        if (current)
            return current;
    }
    // No, get data from next bin
    return analyzeBin();
}

const RawValue::Data *PlotReader::get() const
{   return current; }

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

