#ifndef __GNUPLOTEXPORTER_H__
#define __GNUPLOTEXPORTER_H__

#include "SpreadSheetExporter.h"

BEGIN_NAMESPACE_CHANARCH

//CLASS GNUPlotExporter
// GNUPlotExporter generates a text file for GNUPlot
// together with a simple plotting script.
//
// The data file will be called "<filename>",
// the script will be called "<filename>.plt".
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

	//* When set, this exporter will create a script
	// for GNUPlot that will build "<filename>.png"
	// instead of displaying the plot in a window.
	void makeImage ()
	{	_make_image = true;	}

	static const char *imageExtension ()
	{	return ".png";	}

	//* When set, this exporter will create a script
	// for GNUPlot that will set the path
	// to where the script is.
	// (useful when called from someplace else
	// since the script wouldn't find the data files)
	void setPath ()
	{	_set_path = true;	}

protected:
	void prolog (ostream &out);
	void post_scriptum (const vector<stdString> &channel_names);

private:
	stdString _script_name;
	bool _make_image;
	bool _set_path;
};

END_NAMESPACE_CHANARCH


#endif //__GNUPLOTEXPORTER_H__
