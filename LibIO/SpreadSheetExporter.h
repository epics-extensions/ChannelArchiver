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
    SpreadSheetExporter(ArchiveI *archive);
    SpreadSheetExporter(ArchiveI *archive, const stdString &filename);

protected:
    void prolog(std::ostream &out);
};


#endif //__SPREADSHEETEXPORTER_H__


