// MatlabExporter.h

#ifndef __MATLAB_EXPORTER__
#define __MATLAB_EXPORTER__

#include "Exporter.h"

//CLASS MatlabExporter
// Generate a text file that's readable
// by Matlab (version 4.2 or newer).
//
// Implements CLASS Exporter.
class MatlabExporter : public Exporter
{
public:
    MatlabExporter(ArchiveI *archive, const stdString &filename);

    virtual void exportChannelList(const stdVector<stdString> &channel_names);
protected:
};

#endif
