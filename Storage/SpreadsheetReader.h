// -*- c++ -*-

// Tools
#include "ToolsConfig.h"
// Storage
#include "DataReader.h"
// rtree
#include "archiver_index.h"
/// \ingroup Storage
/// @{

/// Reads data from storage in form suitable for spreadsheets

/// Based on several DataReader classes, which reach read a single
/// channel, the SpreadsheetReader reads multiple channels,
/// stepping through the values in time such that one can
/// use them for spreadsheet-type output, one point in time
/// per line.
class SpreadsheetReader
{
  public:
    SpreadsheetReader(archiver_Index &index);

    virtual ~SpreadsheetReader();

    /// Position the reader on-or-before start time
    /// for all channels.
    /// Returns true if values are valid.
    bool find(const stdVector<stdString> &channel_names,
              const epicsTime *start);

    /// Number of array entries for the following stuff that's non-scalar
    size_t num;
    
    /// The current time slice
    epicsTime time;
    
    /// The current values.

    /// Individual entries might be 0 if there's
    /// no data for the current time slice
    const RawValue::Data **data;
    
    /// Get the next time slice
    bool next();
    
protected:
    archiver_Index &index;
    DataReader **reader;
};
