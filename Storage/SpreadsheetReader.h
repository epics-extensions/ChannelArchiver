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
    /// It's a severe error to invoke any of the following
    /// after find() returns false.
    bool find(const stdVector<stdString> &channel_names,
              const epicsTime *start);

    /// Time stamp for the current slice of data
    const epicsTime &getTime() const;
    
    /// Number of entries in the following arrays.

    /// Should match the size of the channel_names array passed to find().
    /// It is a severe error to invoke getName(), getValue() etc.
    /// with an index outside of 0...getNum()-1.
    size_t getNum() const;

    /// Returns name of channel i=0...getNum()-1.
    const stdString &getName(size_t i) const;

    /// Returns value of channel i=0...getNum()-1.

    /// The result might be 0 in case a channel
    /// does not have a valid value for the current
    /// time slice.
    const RawValue::Data *getValue(size_t i) const;

    /// The dbr_time_xxx type
    DbrType getType(size_t i) const;
    
    /// array size
    DbrCount getCount(size_t i) const;
    
    /// The meta information for the channel
    const CtrlInfo &getCtrlInfo(size_t i) const;
    
    /// Get the next time slice
    bool next();  
  
protected:
    archiver_Index &index;

    // Number of array entries for the following stuff that's non-scalar
    size_t num;

    // One reader per channel (also has a copy of the channel names)
    DataReader **reader;

    // Current data for each reader.
    const RawValue::Data **read_data;
    
    // The current time slice
    epicsTime time;
    
    // The current values.
    const RawValue::Data **value;
};

inline const epicsTime &SpreadsheetReader::getTime() const
{    return time; }

inline size_t SpreadsheetReader::getNum() const
{   return num; }

inline const stdString &SpreadsheetReader::getName(size_t i) const
{   return reader[i]->channel_name; }

inline const RawValue::Data *SpreadsheetReader::getValue(size_t i) const
{   return value[i]; }

inline DbrType SpreadsheetReader::getType(size_t i) const
{   return reader[i]->dbr_type; }
    
inline DbrCount SpreadsheetReader::getCount(size_t i) const
{   return reader[i]->dbr_count; }

inline const CtrlInfo &SpreadsheetReader::getCtrlInfo(size_t i) const
{   return reader[i]->ctrl_info; }


