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
#include "ArchiveException.h"

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
    _linear_interpol_secs = 0.0;
    _gap_factor = 0;
    _fill = false;
    _be_verbose = false;
    _is_array = false;
    _max_channel_count = 100;
    _data_count = 0;
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
}

void Exporter::printValue(std::ostream *out,
                          const osiTime &time, const ValueI *v)
{
    // skip values which are Archiver specials
    size_t ai;
    stdString txt;
    if (v->isInfo())
    {
        for (ai=0; ai<v->getCount(); ++ai)
            *out << '\t' << _undefined_value;
    }
    else
    {
        const CtrlInfoI *info = v->getCtrlInfo();
        
        // Format according to precision.
        // Unfortuately that is usually configured wrongly
        // and then people complain about not seeing their data...
        if (v->getType() == DBR_TIME_STRING  ||
            (info && info->getType() == CtrlInfoI::Enumerated))
        {
            v->getValue(txt);
            *out << '\t' << txt;
        }
        else
        {
            for (ai=0; ai<v->getCount(); ++ai)
                *out << '\t' << v->getDouble(ai);
        }
    }

    if (_show_status)
    {
        v->getStatus(txt);
        if (txt.empty())
            *out << "\t ";
        else
            *out << '\t' << txt;
    }
}



