// GNUPlotExporter.cpp

#ifdef WIN32
#pragma warning (disable: 4786)
#endif

#include "ArchiveException.h"
#include "GNUPlotExporter.h"
#include "Filename.h"
#include <fstream>

USING_NAMESPACE_STD
BEGIN_NAMESPACE_CHANARCH

GNUPlotExporter::GNUPlotExporter (Archive &archive, const stdString &filename)
: SpreadSheetExporter (archive.getI(), filename)
{
	_make_image = false;
	_set_path = false;
	_time_col_val_format = true;
	if (filename.empty())
		throwDetailedArchiveException(Invalid, "empty filename");
}           

GNUPlotExporter::GNUPlotExporter (ArchiveI *archive, const stdString &filename)
: SpreadSheetExporter (archive, filename)
{
	_make_image = false;
	_set_path = false;
	_time_col_val_format = true;
	if (filename.empty())
		throwDetailedArchiveException(Invalid, "empty filename");
}           

// Will be called before actually outputting anything.
// This routine might write header information
// and adjust the following settings for this Exporter
void GNUPlotExporter::prolog (ostream &out)
{
	stdString script_base;
	Filename::getBasename (_filename, script_base);
	script_base += ".plt";
	_script_name = _filename;
	_script_name += ".plt";

	out << "# Generated from archive data\n";
	out << "#\n";
	out << "# A GNUPlot-script '" << script_base << "' has\n";
	out << "# been generated to plot the data in this file.\n";
	out << "#\n";
	out << "#"; // comment out the following header line

	_undefined_value = "#N/A";
}

// Will be called after dumping the actual values
// while the main data file is still open.
void GNUPlotExporter::post_scriptum (const vector<stdString> &channel_names)
{
	stdString dir;
	stdString data;
	stdString image;
	Filename::getDirname  (_filename, dir);
	Filename::getBasename (_filename, data);
	image = data;
	image += imageExtension ();

	ofstream script (_script_name.c_str ());
	if (!script.is_open())
		throwDetailedArchiveException (CreateError, _script_name);

	script << "# This GNUPlot-script  has been generated\n";
	script << "# to plot the data in '" << data << "'\n";
	script << "#\n";
	script << "\n";

	if (_set_path)
	{
		script << "# Change to the data directory:\n";
		script << "cd '" << dir << "'\n\n";
	}

	if (_make_image)
	{
		script << "# Create image:\n";
		script << "set terminal png small color\n";
		script << "set output \"" << image << "\"\n";
		script << "\n";
	}

	script << "# x-axis is time axis:\n";
	script << "set xdata time\n";
	script << "set timefmt \"%m/%d/%Y %H:%M:%S\"\n";
	if (_is_array)
		script << "set format x \"%m/%d/%Y, %H:%M:%S\"\n";
	else
		script << "set format x \"%m/%d/%Y\\n%H:%M:%S\"\n";
	script << "\n";
	script << "# Format, you might want to change these:\n";
	script << "set pointsize 0.5\n";
	script << "# Set labels/ticks on x-axis for every xxx seconds:\n";

	double secs = (double(_end) - double(_start)) / 5;
	if (secs > 0.0)
		script << "set xtics " << secs << "\n";
	script << "set grid\n";
	script << "\n";
	script << "set key title \"Channels:\"\n";
	script << "set key box\n";

	size_t num = channel_names.size();
	if (num <= 0)
		script << "# funny, there is no data ?\n";
	else
	{
		if (_is_array)
		{
			script << "set view 60,260,1\n";
			script << "#set contour\n";
			script << "set surface\n";
			if (_datacount < 50)
				script << "set hidden3d\n";
			else
				script << "#set hidden3d\n";

			script << "splot ";
			script << '"' << data << "\" using 1:3:4"
				<< " title \"" << channel_names[0] << "\"";
			if (_datacount < 30)
				script << "with linespoints\n";
			else
				script << "with lines\n";
		}
		else
		{
			script << "plot ";

			for (size_t i=0; i<num; ++i)
			{
				script << '"' << data << "\" using 1:" << i+3
					<< " title \"" << channel_names[i] << "\" with linespoints";
				if (i < num-1)
					script << ", ";
			}
			script << "\n";
		}
	}
	script << "\n";
	if (!_make_image)
		script << "pause -1  \"Hit return to continue\"\n";
}

END_NAMESPACE_CHANARCH
