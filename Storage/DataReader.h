// -*- c++ -*-

// Tools
#include "stdString.h"
// Storage
#include "DirectoryFile.h"
#include "CtrlInfo.h"
#include "RawValue.h"

/// \ingroup Storage
/// @{

/// Reads data from storage.

/// The data reader interfaces to the Index/DataFile
/// and returns a stream of RawValue values.
///
class DataReader
{
public:
    /// Create a reader for an index.
    DataReader(DirectoryFile &index);

    ~DataReader();
    
    /// Locate data.

    /// Positions reader on given channel and start time.
    ///
    /// Specifically: If a value with the exact start time exists,
    /// it will be returned. Otherwise the value just before the start time
    /// is returned, so that the user can then decide if and how that value
    /// might extrapolate onto the start time.
    ///
    /// \param channel_name: Name of the channel
    /// \param start: start time or 0 for first value
    /// \return Returns value or 0
    const RawValue::Data *find(const stdString &channel_name,
                               const epicsTime *start);

    /// Returns next value or 0.
    const RawValue::Data *next();
    
    /// The dbr_time_xxx type
    DbrType dbr_type;
    
    /// array size
    DbrCount dbr_count;
    
    /// The meta information for the channel
    CtrlInfo ctrl_info;

    /// next() updates this if dbr_type/count changed.
    bool type_changed;

    /// next() updates this if ctrl_info changed.
    bool ctrl_info_changed;
    
    double period;    

private:
    DirectoryFile &index;
    DataFile *datafile;
    stdString channel_name;
    RawValue::Data *data;
    size_t raw_value_size;
    class DataHeader *header;
    size_t val_idx; // current index in data buffer

    DataHeader *getHeader(const stdString &dirname,
                          const stdString &basename,
                          FileOffset offset);
};

/// @}

