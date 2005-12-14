// -*- c++ -*-

#ifndef __DATA_READER_H__
#define __DATA_READER_H__

// Tools
#include "stdString.h"
#include "ErrorInfo.h"
// Storage
#include "CtrlInfo.h"
#include "RawValue.h"
#include "Index.h"
#include "RTree.h"

/// \addtogroup Storage
/// @{

/// Reads data from storage.

/// The data reader interfaces to the Index/DataFile
/// and returns a stream of RawValue values.
///
class DataReader
{
public:
    DataReader();
    virtual ~DataReader();
    
    /// Locate data.

    /// Positions reader on given channel and start time.
    ///
    /// Specifically: If a value with the exact start time exists,
    /// it will be returned. Otherwise the value just before the start time
    /// is returned, so that the user can then decide if and how that value
    /// might extrapolate onto the start time.
    ///
    /// ErrorInfo should indicate file access error or the fact
    /// that a channel doesn't exist.
    /// If the channel exists but there simply is no data,
    /// error_info will be clear yet 0 data will be returned.
    ///
    /// \param channel_name: Name of the channel
    /// \param start: start time or 0 for first value
    /// \param error_info: may be set to error information
    /// \return Returns value or 0
    virtual const RawValue::Data *find(const stdString &channel_name,
                                       const epicsTime *start,
                                       ErrorInfo &error_info) = 0;

    /// \param error_info: may be set to error information
    /// \return Returns next value or 0.
    virtual const RawValue::Data *next(ErrorInfo &error_info) = 0;

    /// Name of the channel, i.e. the one passed to find()
    stdString channel_name;
    
    /// The dbr_time_xxx type
    virtual DbrType getType() const = 0;
    
    /// array size
    virtual DbrCount getCount() const = 0;
    
    /// The meta information for the channel
    virtual const CtrlInfo &getInfo() const = 0;

    /// next() updates this if dbr_type/count changed.

    /// Returns whether the type changed or not AND!!
    /// resets the flag!!
    ///
    virtual bool changedType() = 0;

    /// next() updates this if ctrl_info changed.

    /// Returns whether the ctrl_info  changed or not
    /// AND(!) resets the flag!!
    ///
    virtual bool changedInfo() = 0;
};

/// Create one of the DataReader class instances.
class ReaderFactory
{
public:
    /// Determine what DataReader to use:
    enum How
    {
        Raw,     ///< Use RawDataReader
        Plotbin, ///< Use PlotReader
        Average, ///< Use AverageReader
        Linear   ///< Use LinearReader
    };

    /// String representation of how/delta.

    /// The result is suitable for display ala
    /// "Method: ...".
    /// The result is held in a static char buffer,
    /// beware of other threads calling toString.
    static const char *toString(How how, double delta);
    
    /// Create a DataReader.
    static DataReader *create(Index &index, How how, double delta);
};

/// An implementation of the DataReader for the raw data
class RawDataReader : public DataReader
{
public:
    RawDataReader(Index &index);
    virtual ~RawDataReader();
    virtual const RawValue::Data *find(const stdString &channel_name,
                                       const epicsTime *start,
                                       ErrorInfo &error_info);
    virtual const RawValue::Data *next(ErrorInfo &error_info);
    virtual DbrType getType() const;
    virtual DbrCount getCount() const;
    virtual const CtrlInfo &getInfo() const;
    virtual bool changedType();
    virtual bool changedInfo();
private:
    Index     &index;
    stdString directory;
    RTree     *tree;
    RTree::Node *node;// used to iterate
    int rec_idx; 
    bool valid_datablock; // are node/idx on valid datablock? 
    RTree::Datablock datablock; // the current datablock

    DbrType dbr_type;
    DbrCount dbr_count;
    CtrlInfo ctrl_info;
    bool type_changed;
    bool ctrl_info_changed;    
    double period;    

    RawValue::Data *data;
    size_t raw_value_size;
    class DataHeader *header;
    size_t val_idx; // current index in data buffer

    bool getHeader(const stdString &dirname, const stdString &basename,
                   FileOffset offset, ErrorInfo &error_info);
    const RawValue::Data *findSample(const epicsTime &start, ErrorInfo &error_info);
};

/// @}

#endif
