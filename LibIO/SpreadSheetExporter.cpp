// SpreadSheetExporter.cpp

#include "SpreadSheetExporter.h"

SpreadSheetExporter::SpreadSheetExporter(ArchiveI *archive)
    : Exporter(archive)
{}

SpreadSheetExporter::SpreadSheetExporter(ArchiveI *archive,
                                         const stdString &filename)
    : Exporter(archive, filename)
{}


// Will be called before actually outputting anything.
// This routine might write header information
// and adjust the following settings for this Exporter
void SpreadSheetExporter::prolog(std::ostream &out)
{
	out << "; Generated from archive data\n";
	out << ";\n";
	out << "; To plot this data in MS Excel:\n";
	out << "; 1) choose a suitable format for the Time colunm (1st col.)\n";
	out << ";    Excel can display up to the millisecond level\n";
	out << ";    if you choose a custom cell format like\n";
	out << ";              mm/d/yy hh:mm:ss.000\n";
	out << "; 2) Select the table and generate a plot,\n";
	out << ";    a useful type is the 'XY (Scatter) plot'.\n";
	out << ";\n";
    out << "; '#N/A' is used to indicate that there is no valid data for\n";
    out << "; this point in time because e.g. the channel was disconnected.\n";
    out << "; A seperate 'status' column - if included - shows details.\n";
	out << ";\n";

    if (_linear_interpol_secs > 0.0)
	{
		out << "; NOTE:\n";
		out << "; The values in this table\n";
		out << "; were interpolated within " << _linear_interpol_secs << " seconds\n";
		out << "; (linear interpolation), so that the values for different channels\n";
		out << "; can be written on lines for the same time stamp.\n";
		out << "; If you prefer to look at the exact time stamps for each value\n";
		out << "; export the data without interpolation.\n";
	}
	else if (_fill)
	{
		out << "; NOTE:\n";
		out << "; The values in this table were filled, i.e. repeated\n";
		out << "; using staircase interpolation.\n";
		out << "; If you prefer to look at the exact time stamps\n";
		out << "; for all channels, export the data without rounding.\n";
	}
	else
	{
		out << "; This table holds the raw data as found in the archive.\n";
		out << "; Since Channels are not always scanned at exactly the same time,\n";
		out << "; many '#N/A' may appear when looking at more than one channel like this.\n";
	}
	
	out << ";\n";

	_undefined_value = "#N/A";
}

