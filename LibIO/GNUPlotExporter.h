// $id$ -*- c++ -*-
#ifndef __GNUPLOTEXPORTER_H__
#define __GNUPLOTEXPORTER_H__

#include "SpreadSheetExporter.h"

//CLASS GNUPlotExporter
// GNUPlotExporter generates a text file for GNUPlot
// together with a simple plotting script.
//
// The data file will be called "<filename>",
// the script (unless using pipe) will be called "<filename>.plt".
//
// Implements CLASS Exporter.
class GNUPlotExporter : public SpreadSheetExporter
{
public:
	//* Contrary to the simple CLASS Exporter,
	// <I>filename</I> has to be defined
	// for GNUPlotExporter because two files are created
	// and <I>filename</I> is used as a base name.
	GNUPlotExporter (Archive &archive, const stdString &filename);
	GNUPlotExporter (ArchiveI *archive, const stdString &filename);

    void exportChannelList(const stdVector<stdString> &channel_names);

    //* Issue GNUPlot commands to generate an image file
    // (<filename>.png)
    void makeImage()      { _make_image = true; }
    static const char *imageExtension();

    //* Call GNUPlot and run the command script via pipe
    // instead of dumping script to disk
    void usePipe()        { _use_pipe = true; }
    
private:
    bool _make_image;
    bool _use_pipe;
};

#endif //__GNUPLOTEXPORTER_H__
