#ifndef __SPREADSHEETEXPORTER_H__
#define __SPREADSHEETEXPORTER_H__

#include "Exporter.h"

//CLASS SpreadSheetExporter
// Generate a text file that's readable
// by SpreadSheet programs.
//
// Implements CLASS Exporter.
class SpreadSheetExporter : public Exporter
{
public:
    SpreadSheetExporter(Archive &archive, const stdString &filename);
    SpreadSheetExporter(ArchiveI *archive, const stdString &filename);

protected:
    void prolog(std::ostream &out);
};

inline SpreadSheetExporter::SpreadSheetExporter(Archive &archive,
                                                const stdString &filename)
    : Exporter(archive.getI(), filename)
{}

inline SpreadSheetExporter::SpreadSheetExporter(ArchiveI *archive,
                                                const stdString &filename)
    : Exporter(archive, filename)
{}

#endif //__SPREADSHEETEXPORTER_H__
