// -*- c++ -*-

// Tools
#include "ToolsConfig.h"
// Storage
#include "DataReader.h"
// Index
#include "IndexFile.h"

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
    /// Create the SpreadsheetReader

    /// For delta == 0, use the ordinary DataReader.
    /// For delta > 0, use the LinearReader which
    /// attempt linear interpolation or averaging
    /// onto multiples of delta seconds.
    SpreadsheetReader(IndexFile &index, double delta);

    virtual ~SpreadsheetReader();
    
    /// Position the reader on-or-before start time
    /// for all channels.
    /// Returns true if values are valid.
    /// It's a severe error to invoke any of the following
    /// after find() returns false.
    virtual bool find(const stdVector<stdString> &channel_names,
                      const epicsTime *start);

    /// Time stamp for the current slice of data
    virtual const epicsTime &getTime() const;
    
    /// Number of entries in the following arrays.

    /// Should match the size of the channel_names array passed to find().
    /// It is a severe error to invoke getName(), getValue() etc.
    /// with an index outside of 0...getNum()-1.
    virtual size_t getNum() const;

    /// Returns name of channel i=0...getNum()-1.
    virtual const stdString &getName(size_t i) const;

    /// Returns value of channel i=0...getNum()-1.

    /// The result might be 0 in case a channel
    /// does not have a valid value for the current
    /// time slice.
    virtual const RawValue::Data *getValue(size_t i) const;

    /// The dbr_time_xxx type
    virtual DbrType getType(size_t i) const;
    
    /// array size
    virtual DbrCount getCount(size_t i) const;
    
    /// The meta information for the channel
    virtual const CtrlInfo &getCtrlInfo(size_t i) const;
    
    /// Get the next time slice
    bool next();  
  
protected:
    IndexFile &index;

    double delta;
    
    // Number of array entries for the following stuff that's non-scalar
    size_t num;

    // One reader per channel (also has a copy of the channel names)
    DataReader **reader;

    // Current data for each reader.
    // This often already points at the 'next' value.
    const RawValue::Data **read_data;
    
    // The current time slice
    epicsTime time;

    // Copies of the current control infos
    CtrlInfo **info;

    // Type/count for following value
    DbrType *type;
    DbrCount *count;
    
    // The current values, i.e. copy of the reader's value
    // for the current time slice, or 0.
    RawValue::Data **value;
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
{   return reader[i]->getType(); }
    
inline DbrCount SpreadsheetReader::getCount(size_t i) const
{   return reader[i]->getCount(); }

inline const CtrlInfo &SpreadsheetReader::getCtrlInfo(size_t i) const
{   return reader[i]->getInfo(); }


