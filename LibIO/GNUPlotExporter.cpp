// GNUPlotExporter.cpp

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "../ArchiverConfig.h"
#include "ArchiveException.h"
#include "GNUPlotExporter.h"
#include "Filename.h"
#include <fstream>

const char *GNUPlotExporter::imageExtension()
{   return ".png"; }

static void printTime(FILE *f, const osiTime &time)
{
    int year, month, day, hour, min, sec;
    unsigned long nano;
    osiTime2vals (time, year, month, day, hour, min, sec, nano);
    fprintf(f, "%02d/%02d/%04d %02d:%02d:%02d.%09ld",
            month, day, year, hour, min, sec, nano);
}

GNUPlotExporter::GNUPlotExporter (Archive &archive, const stdString &filename)
: SpreadSheetExporter (archive.getI(), filename)
{
    _make_image = false;
    _use_pipe = false;
    if (filename.empty())
        throwDetailedArchiveException(Invalid, "empty filename");
}           

GNUPlotExporter::GNUPlotExporter (ArchiveI *archive, const stdString &filename)
: SpreadSheetExporter (archive, filename)
{
    _make_image = false;
    _use_pipe = false;
    if (filename.empty())
        throwDetailedArchiveException(Invalid, "empty filename");
}           

void GNUPlotExporter::exportChannelList(
    const stdVector<stdString> &channel_names)
{
    size_t num = channel_names.size();
    char info[300];
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

    stdString directory, data_name, script_name, image_name;
    Filename::getDirname  (_filename, directory);
    data_name = _filename;
    script_name = _filename;
    script_name += ".plt";
    image_name = _filename;
    image_name += ".png";
    
    FILE *f;
    f = fopen(data_name.c_str(), "wt");
    if (! f)
        throwDetailedArchiveException(WriteError, data_name);
   
    fprintf(f, "# GNUPlot Exporter V " VERSION_TXT "\n");
    fprintf(f, "#\n");
  
    stdVector<stdString> plotted_channels;
    Archive         archive(_archive);
    ChannelIterator channel(archive);
    ValueIterator   value(archive);
    stdString txt;
    for (size_t i=0; i<num; ++i)
    {
        if (! archive.findChannelByName(channel_names[i], channel))
        {
            archive.detach();
            sprintf(info, "Cannot find channel '%s' in archive",
                    channel_names[i].c_str());
            throwDetailedArchiveException (ReadError, info);
            return;
        }

        channel->getValueAfterTime(_start, value);
        if (! value)
            continue;

        if (value->getCount() > 1)
        {
            _is_array = true;
            if (num > 1)
            {
                archive.detach();
                sprintf(info,
                        "Array channels like '%s' can only be exported "
                        "on their own, "
                        "not together with another channel",
                        channel_names[i].c_str());
                throwDetailedArchiveException(Invalid, info);
            }
        }
        
        // Header: Channel name [units]
        fprintf(f, "# %s [%s]\n",
                channel->getName(),
                value->getCtrlInfo()->getUnits());
        
        // Dump values for this channel
        osiTime time;
        bool have_anything = false;
        while (value)
        {
            time = value->getTime();
            if (isValidTime(_end)  && time > _end)
                break;
            if (value->isInfo())
            {
                // Show as comment, empty line results in "gap"
                // in GNUplot graph
                fprintf(f, "# ");
                ::printTime(f, time);
                value->getStatus(txt);
                fprintf(f, "\t%s\n\n", txt.c_str());
            }
            else
            {
                have_anything = true;
                ++_data_count;
                if (_is_array)
                {
                    for (size_t ai=0; ai<value->getCount(); ++ai)
                    {
                        ::printTime(f, time);
                        fprintf(f, "\t%g\n", value->getDouble(ai));
                    }
                }
                else
                {
                    ::printTime(f, time);
                    value->getStatus(txt);
                    fprintf(f, "\t%g\t%s\n",
                            value->getDouble(),
                            txt.c_str());
                }
            }
            ++value;
        }
        if (have_anything)
            plotted_channels.push_back(channel->getName());
        fprintf(f, "\n\n\n");
    }
    archive.detach();
    fclose(f);

    // Generate script
    if (_use_pipe)
    {
        f = popen(GNUPLOT_PROGRAM, "w");
        if (!f)
            throwDetailedArchiveException(OpenError, GNUPLOT_PROGRAM);
    }
    else
    {
        fopen(script_name.c_str(), "wt");
        if (!f)
            throwDetailedArchiveException(WriteError, script_name);
    }

    fprintf(f, "# GNUPlot Exporter V " VERSION_TXT "\n");
    fprintf(f, "#\n\n");
    if (!directory.empty())
    {
        fprintf(f, "# Change to the data directory:\n");
        fprintf(f, "cd '%s'\n", directory.c_str());
    }
    if (_make_image)
    {
        fprintf(f, "# Create image:\n");
        fprintf(f, "set terminal png small color\n");
        fprintf(f, "set output '%s'\n", image_name.c_str());
    }

    fprintf(f, "# x-axis is time axis:\n");
    fprintf(f, "set xdata time\n");
    fprintf(f, "set timefmt \"%%m/%%d/%%Y %%H:%%M:%%S\"\n");
    if (_is_array)
        fprintf(f, "set format x \"%%m/%%d/%%Y, %%H:%%M:%%S\"\n");
    else
        fprintf(f, "set format x \"%%m/%%d/%%Y\\n%%H:%%M:%%S\"\n");
    fprintf(f, "\n");
#if 0
    fprintf(f, "# Set labels/ticks on x-axis for every xxx seconds:\n");
    double secs = (double(_end) - double(_start)) / 5;
    if (secs > 0.0)
        fprintf(f, "set xtics %g\n", secs);
#endif
    fprintf(f, "set grid\n");
    fprintf(f, "\n");
    fprintf(f, "set key title \"Channels:\"\n");
    fprintf(f, "set key box\n");

    num = plotted_channels.size();
    if (num == 2)
    {
        fprintf(f, "# When using 2 channels:\n");
        fprintf(f, "set ylabel '%s'\n", plotted_channels[0].c_str());
        fprintf(f, "set y2label '%s'\n", plotted_channels[1].c_str());
        fprintf(f, "set y2tics\n");
    }
    
    if (_is_array)
    {
        fprintf(f, "set view 60,260,1\n");
        fprintf(f, "#set contour\n");
        fprintf(f, "set surface\n");
        if (_data_count < 50)
            fprintf(f, "set hidden3d\n");
        else
            fprintf(f, "#set hidden3d\n");
        fprintf(f, "splot '%s' using 1:3:4",
                data_name.c_str());
        fprintf(f, " title '%s' with lines\n",
                plotted_channels[0].c_str());
    }
    else
    {
        fprintf(f, "plot ");
        for (size_t i=0; i<num; ++i)
        {
            fprintf(f, "'%s' index %d using 1:3",
                    data_name.c_str(), i);
            if (num==2 && i==1)
                fprintf(f, " axes x1y2");
            fprintf(f, " title '%s' with steps",
                    plotted_channels[i].c_str());
            if (i < num-1)
                fprintf(f, ", ");
        }
        fprintf(f, "\n");
    }
    if (_use_pipe)
        pclose(f);
    else
        fclose(f);
}






