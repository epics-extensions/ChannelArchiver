// Exporter.h -*- c++ -*-

#ifndef __EXPORTER_H__
#define __EXPORTER_H__

#include "ArchiveI.h"
#include <iostream>

//CLASS Exporter
// Exporter is the virtual base class for
// tools that export data from a ChannelArchive:
// <UL>
// <LI>CLASS SpreadSheetExporter
// <LI>CLASS GNUPlotExporter
// <LI>CLASS MatlabExporter
// </UL>
//
// These classes export the values of selected channels
// as text files.
// Each derived class generates a slightly different
// type of text file for the specific target application.
class Exporter
{
public:
    //* All Exporters work on an open Archive.
    // <I>filename</I> is the (base)name
    // for the generated file.
    // When left empty, <I>cout</I> is used.
    //
    // See also: CLASS FilenameTool
    Exporter(ArchiveI *archive);
    Exporter(ArchiveI *archive, const stdString &filename);

    virtual ~Exporter() {}

    //* Allowed number of channels to export
    // (limited for performance reason)
    void setMaxChannelCount(size_t limit);

    //* Set start/end time. Default: dump from whole archive
    void setStart(const osiTime &start);
    void setEnd(const osiTime &end);

    //* When 'secs' is positive,
    // all timestamps within 'secs' will be
    // regarded as the same point in time
    void setTimeRounding(double secs);

    //* Switch on linear interpolation,
    // generating a value every "secs".
    // Gaps bigger than "secs * gap" will be
    // shown as "Archive_Off"
    // to avoid endless interpolation.
    void setLinearInterpolation(double secs, size_t gap = 0);

    //* When using filled values,
    // missing entries (when the value has not changed since the last entry)
    // will be filled in by repeating the last value
    void useFilledValues();

    void setVerbose(bool verbose = true)
    {   _be_verbose = verbose; }

    //* Generate extra column for each channel
    // with status information?
    void enableStatusText(bool yesno = true);

    //* Export channels that match a name pattern
    void exportMatchingChannels(const stdString &channel_name_pattern);

    //* Export channels from provided list
    virtual void exportChannelList(const stdVector<stdString> &channel_names);

    //* Return number of data lines produced
    size_t getDataCount();

protected:
    ArchiveI *_archive;
    stdString _filename;
    osiTime _start, _end;
    double _round_secs;
    double _linear_interpol_secs;
    size_t _gap_factor;
    bool _fill;
    bool _be_verbose;

    void printTime(std::ostream *out, const osiTime &time);
    void printValue(std::ostream *out, const osiTime &time, const ValueI *v);

    // Will be called before actually outputting anything.
    // This routine might write header information
    // and adjust the following settings for this Exporter
    virtual void prolog(std::ostream &out) {}

    // Settings:

    // String to be used for undefined values
    stdString _undefined_value;
    // Show status text in seperate column
    bool _show_status;

    // For GNUPlot: each line holds
    // Time Column# Value
    bool _time_col_val_format;

    bool _is_array;
    size_t _datacount;
    size_t _max_channel_count;

    // Will be called after dumping the actual values.
    virtual void post_scriptum(const stdVector<stdString> &channel_names) {}
private:
    void init(ArchiveI *archive);
};

inline void Exporter::setStart(const osiTime &start)   { _start = start; }
inline void Exporter::setEnd(const osiTime &end)       { _end = end; }
inline void Exporter::enableStatusText(bool yesno)     { _show_status = yesno; }
inline size_t Exporter::getDataCount()                 { return _datacount; }

inline void Exporter::setTimeRounding(double secs)
{
    _fill = false;
    _round_secs = secs;
    _linear_interpol_secs = 0.0;
}

inline void Exporter::setLinearInterpolation(double secs, size_t gap)
{
    _fill = false;
    _round_secs = 0.0;
    _linear_interpol_secs = secs;
    _gap_factor = gap;
}

inline void Exporter::useFilledValues()
{   
    _fill = true;
    _round_secs = 0.0;
    _linear_interpol_secs = 0.0;
}

inline void Exporter::setMaxChannelCount(size_t limit)
{
    _max_channel_count = limit;
}


#endif //__EXPORTER_H__
