// $Id$
//
// MatlabExporter.cpp

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "MatlabExporter.h"
#include "ArchiveException.h"
#include <fstream>
#include <iostream>

bool MatlabExporter::osiTime2datestr(const osiTime &time, char *text)
{
    int year, month, day, hour, min, sec;
    unsigned long nano;
    
    osiTime2vals (time, year, month, day, hour, min, sec, nano);
    sprintf(text, "%02d/%02d/%04d %02d:%02d:%02d.%09ld",
            month, day, year, hour, min, sec, nano);
    return true;
}

bool MatlabExporter::datestr2osiTime(const char *text, osiTime &time)
{
    int     year, month, day, hour, min;
    double  second;

    if (sscanf(text, "%02d/%02d/%04d %02d:%02d:%lf",
               &month, &day, &year, &hour, &min, &second) != 6)
        return false;

    int secs = (int)second;
    unsigned long nano = (unsigned long) ((second - secs) * 1000000000L);

    vals2osiTime(year, month, day, hour, min, secs, nano, time);

    return true;
}

MatlabExporter::MatlabExporter(ArchiveI *archive)
        : Exporter(archive)
{
}

MatlabExporter::MatlabExporter(ArchiveI *archive, const stdString &filename)
        : Exporter(archive, filename)
{
}

void MatlabExporter::exportChannelList(
    const stdVector<stdString> &channel_names)
{
    size_t i, ai, count, line, num = channel_names.size();
    Archive         archive(_archive);
    ChannelIterator channel(archive);
    ValueIterator   value(archive);
    osiTime         time;
    stdString       txt;
    char            datestr[DATESTR_LEN];
    char            info[300];

    if (channel_names.size() > _max_channel_count)
    {
        archive.detach();
        sprintf(info,
                "You tried to export %d channels.\n"
                "For performance reason you are limited to export %d "
                "channels at once",
                channel_names.size(), _max_channel_count);
        throwDetailedArchiveException(Invalid, info);
        return;
    }

    // Redirection of output
    std::ostream *out = &std::cout; // default: stdout
    std::ofstream file;
    if (! _filename.empty())
    {
        file.open (_filename.c_str());
#       if defined(HP_UX)
        if (file.fail())
#else
        if (! file.is_open())
#endif
        {
            archive.detach();
            throwDetailedArchiveException(WriteError, _filename);
        }
        out = &file;
    }

    *out << "% MatLab data file, created by ChannelArchiver.\n";
    *out << "% Channels: "  "\n";
    for (i=0; i<num; ++i)
        *out << "%  " << channel_names[i] << "\n";
    *out << "%\n";
    *out << "% Struct: t - time string\n";
    *out << "%         v - value\n";
    if (_show_status)
        *out <<  "%         s - status\n";
    *out << "%         d - date number\n";
    *out << "%         l - length of data\n";
    *out << "%         n - name\n";
    *out << "%\n";
    *out << "% Example for generic plot func. that can handle this data:\n";
    *out << "%\n";
    *out << "%  function archdataplot(data)\n";
    *out << "%\n";
    *out << "%  plot(data.d, data.v);\n";
    *out << "%  datetick('x');\n";
    *out << "%  xlabel([data.t{1} ' - ' data.t{data.l}]);\n";
    *out << "%  title(data.n);\n";

    
    for (i=0; i<num; ++i)
    {
        if (! archive.findChannelByName(channel_names[i], channel))
        {
            *out << "% Cannot find channel '" << channel_names[i]
                 << "' in archive\n";
            continue;
        }
        if (! channel->getValueBeforeTime(_start, value) &&
            ! channel->getValueAfterTime(_start, value))
        {
            *out << "% No values found for channel '" << channel_names[i]
                 << "'\n";
            continue;
        }
        
        char *p, *variable = strdup(channel_names[i].c_str());
        bool fixed_name = false;
        while ((p=strchr(variable, ':')) != 0)
        {
            *p = '_';
            fixed_name = true;
        }
        if (fixed_name)
            *out << "% Used '" << variable
                 << "' for channel '" << channel_names[i] << "'\n";

        // Value loop per channel
        line = 0;
        count = value->getCount();
        long o_flags = out->flags();
        long o_prec = out->precision();
        while (value)
        {
            time=value->getTime();
            ++line;
            ++_data_count;
            osiTime2datestr(time, datestr);
            sprintf(info, "%s.t(%d)={'%s'};", variable, line, datestr);
            *out << info << "\n";
        
            if (value->isInfo())
                *out << variable << ".v(" << line << ")=nan;\n";
            else
            {
                const CtrlInfoI *info = value->getCtrlInfo();
                if (info && info->getPrecision() > 0)
                {
                    out->flags(std::ios::fixed);
                    out->precision(info->getPrecision());
                }
                if (count == 1)
                    *out << variable << ".v(" << line << ")="
                         << value->getDouble() << ";\n";
                else
                {
                    *out << variable << ".v(" << line << ")=[";
                    for (ai=0; ai<count; ++i)
                    {
                        *out << value->getDouble(ai);
                        if (ai != count-1)
                            *out << ",";
                    }
                    *out << "];\n";
                }
            }
            if (_show_status)
            {
                value->getStatus (txt);
                *out << variable << ".s(" << line << ")={'"
                     << txt << "'};\n";
            }
            // Show one value after _end, then quit:
            if (isValidTime(_end) && time >= _end)
                break;
            ++value;
        }
        out->flags(o_flags);
        out->precision(o_prec);

        *out << variable << ".d=datenum(char("
             << variable << ".t));\n";
        *out << variable << ".l=size("
             << variable << ".v, 2);\n";
        *out << variable << ".n='"
             << variable << "';\n";
        free(variable);
    }
    
    if (out == &file)
        file.close();
    archive.detach();
}

