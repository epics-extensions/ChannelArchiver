// Export.cpp

#include "../ArchiverConfig.h"
#include "BinArchive.h"
#include "MultiArchive.h"
#include "GNUPlotExporter.h"
#include "MatlabExporter.h"
#include "ArgParser.h"

int main(int argc, const char *argv[])
{
    CmdArgParser parser(argc, argv);
    parser.setArgumentsInfo(" <directory file> { channel }");
    CmdArgString start_time (parser,
                             "start", "<time>", "Format: \"mm/dd/yy[ hh:mm:ss[.nano-secs]]\"");
    CmdArgString end_time   (parser,
                             "end", "<time>", "(exclusive)");
    CmdArgFlag   fill       (parser,
                             "fill", "fill columns w/ repeated values");
    CmdArgDouble interpol   (parser,
                             "interpolate", "<seconds>", "interpolate values");
    CmdArgFlag   verbose    (parser,
                             "verbose", "verbose mode");
    CmdArgFlag   GNUPlot    (parser,
                             "gnuplot", "generate GNUPlot output");
    CmdArgInt    reduce     (parser,
                             "reduce", "<# of buckets>", "reduce data to # buckets");
    CmdArgFlag   image      (parser,
                             "Gnuplot", "generate GNUPlot output for Image");
    CmdArgFlag   pipe       (parser,
                             "pipe", "run GNUPlot via pipe");
    CmdArgFlag   MLSheet    (parser,
                             "MLSheet", "generate spreadsheet for Matlab");
    CmdArgFlag   Matlab     (parser,
                             "Matlab", "generate Matlap command-file output");
    CmdArgFlag   status_text(parser,
                             "text", "include status information");
    CmdArgString output     (parser,
                             "output", "<file>", "output to file");
    CmdArgString pattern    (parser,
                             "match", "<reg. exp.>", "channel name pattern");
    if (! parser.parse())
        return -1;
    if (parser.getArguments().size() < 1)
    {
        fputs("ArchiveExport " VERSION_TXT "\n\n", stderr);
        parser.usage();
        return -1;
    }

	if (reduce > 0  && ! GNUPlot)
    {
        fputs("Error:\n"
              "Reduce option works only for GNUPlot output\n", stderr);
        parser.usage();
        return -1;
    }
    if (image || pipe)
        GNUPlot.set();
    if (GNUPlot  &&  output.get().empty())
    {
        fputs("Error:\n"
              "You must specify an output file (-o) for GNUPlot output\n", stderr);
        parser.usage();
        return -1;
    }

    try
    {
        osiTime startTime, endTime;
        // start and/or end time provided ?
	if (! start_time.get().empty()) string2osiTime(start_time, startTime);
	if (! end_time.get().empty()) string2osiTime(end_time, endTime);

        ArchiveI *archive = new EXPORT_ARCHIVE_TYPE(parser.getArgument (0),
						    startTime, endTime);
        Exporter *exporter;
        
        if (GNUPlot)
        {
            GNUPlotExporter *gnu = new GNUPlotExporter(archive, output, int(reduce));
            if (image)
                gnu->makeImage();
            if (pipe)
                gnu->usePipe();
            exporter = gnu;
        }
        else if (Matlab)
        {
            MatlabExporter *matlab = new MatlabExporter(archive, output);
            exporter = matlab;
        }
        else
        {
            SpreadSheetExporter *sse = new SpreadSheetExporter(archive, output);
            if (MLSheet)
                sse->useMatlabFormat();
            exporter = sse;
        }

#ifdef EXTEND_EXPORT
     exporter->setMaxChannelCount(1000);
#endif
        exporter->setVerbose(verbose);
        if (status_text)
            exporter->enableStatusText();
        if (bool(fill))
            exporter->useFilledValues();
        if (double(interpol) > 0)
            exporter->setLinearInterpolation(interpol, 100);
        if (isValidTime(startTime))
            exporter->setStart(startTime);
        if (isValidTime(endTime))
            exporter->setEnd(endTime);
        // List of channels given?
        if (parser.getArguments().size() > 1)
        {   // yes, use it
            stdVector<stdString> channel_names;
            if (! pattern.get().empty())
            {
                fputs("Pattern from '-m' switch is ignored\n"
                      "since a list of channels was also provided\n", stderr);
            }
            // first argument was directory file name, skip that:
            for (size_t i=1; i<parser.getArguments().size(); ++i)
                channel_names.push_back(parser.getArgument(i));
            exporter->exportChannelList(channel_names);
        }
        else
            exporter->exportMatchingChannels(pattern);
        delete archive;
    }
    catch (const GenericException &e)
    {
        fprintf(stderr, "Exception:\n%s\n", e.what());
        LOG_MSG("Exception:\n%s\n", e.what());
        return -1;
    }

    return 0;
}
