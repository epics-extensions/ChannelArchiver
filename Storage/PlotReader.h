// -*- c++ -*-

#ifndef __PLOT_READER_H__
#define __PLOT_READER_H__

#include "RawDataReader.h"

/** \ingroup Storage
 * Reads data from storage, modified for plotting.
 *
 * The PlotReader is an implementaion of a DataReader
 * that returns data in a format suitable for plotting.
 * 
 * Beginning at the requested start time, the raw samples
 * within the next bin of 'delta' seconds are investigated, locating the
 * - initial,
 * - minimum,
 * - maximum,
 * - final value
 * within the bin as well as the last 'info' sample
 * like 'disconnected' or 'archive off'.
 * 
 * Those 5 samples are then sorted in time: Initial and final stay
 * as they are, but minimum, maximum and info samples might occur at
 * any time within the bin.
 * 
 * Finally, a 'unique' filter is applied:
 * If the initial and minimum sample are one and the same,
 * only the initial sample is returned. Similarly, the might only be a single
 * intial == mini == maxi == final sample, so only that single sample
 * is returned.
 * 
 * Then the next bin is investigated.
 * 
 * Note that there is no indication which value we're currently
 * returning: The first call to find() will investigate the current
 * bin and return the inital value. The following call to next() might
 * return the minumum within the bin, then the maximum and so on.
 *
 * This is meant to feed a plotting tool, one that simply draws a line
 * from sample to sample, with the intention of showing an envelope of the
 * raw data, resulting in significant data reduction.
 */
class PlotReader : public DataReader
{
public:
    /** Create a reader for an index.
     *
     * delta == 0 causes it to behave like the RawDataReader.
     * @param delta The bin size in seconds.
     */
    PlotReader(Index &index, double delta);
    const RawValue::Data *find(const stdString &channel_name,
                               const epicsTime *start);
    const RawValue::Data *next();
    const RawValue::Data *get() const;
    DbrType getType() const;
    DbrCount getCount() const;
    const CtrlInfo &getInfo() const;
    bool changedType();
    bool changedInfo();
private:
    /** Bin size in seconds. */
    double delta;
    /** End of the current bin. */
    epicsTime end_of_bin;
    
    /** Base reader. */
    RawDataReader reader;

    /** Current value of underlying base reader */
    const RawValue::Data *reader_data;

    /** Values of this PlotReader */
    DbrType type;
    DbrCount count;
    bool type_changed;
    RawValueAutoPtr initial_sample, mini_sample, maxi_sample, info_sample, final_sample;
    bool have_initial, have_mini, have_maxi, have_info, have_final;
    double mini_dbl, maxi_dbl;

    CtrlInfo info;
    bool ctrl_info_changed;

    enum { BinSampleCount = 5 };
    const RawValue::Data *samples[BinSampleCount];
    int sample_index;
    
    /** Add sample to _samples_, inserting in time order, but ignoring dups. */
    void addSample(const RawValue::Data *);

    const RawValue::Data *current;

    /** Set to "we have nothing" */
    void clearValues();

    /** Analyze the next bin. */
    const RawValue::Data *analyzeBin();
};

#endif
