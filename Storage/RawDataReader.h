// -*- c++ -*-

#ifndef __RAW_DATA_READER_H__
#define __RAW_DATA_READER_H__

// Tools
#include <ToolsConfig.h>
#include <AutoPtr.h>
// Storage
#include "DataReader.h"

/// \addtogroup Storage
/// @{

/** An implementation of the DataReader for raw data.
 *  <p>
 *  It reads the original samples from a 'full' index.
 *  No averaging, no support for 'shallow' indices.
 */
class RawDataReader : public DataReader
{
public:
    RawDataReader(Index &index);
    virtual ~RawDataReader();
    virtual const RawValue::Data *find(const stdString &channel_name,
                                       const epicsTime *start);
    virtual const stdString &getName() const;                                   
    virtual const RawValue::Data *next();
    virtual const RawValue::Data *get() const;
    virtual DbrType getType() const;
    virtual DbrCount getCount() const;
    virtual const CtrlInfo &getInfo() const;
    virtual bool changedType();
    virtual bool changedInfo();
private:
    // Index
    Index                     &index;
    // Channel found in index
    AutoPtr<Index::Result>    index_result;  
    // Current data block info for that channel  
    AutoPtr<RTree::Datablock> datablock;
    
    // Name of channel from last find() call
    stdString channel_name;
    
    // Meta-info for current sample    
    DbrType dbr_type;
    DbrCount dbr_count;
    CtrlInfo ctrl_info;
    bool type_changed;
    bool ctrl_info_changed;    
    double period;    

    // Current sample
    RawValueAutoPtr data;
    size_t raw_value_size;
    AutoPtr<class DataHeader> header;
    size_t val_idx; // current index in data buffer

    void getHeader(const stdString &basename, FileOffset offset);
    const RawValue::Data *findSample(const epicsTime &start);
    const RawValue::Data *nextFromDatablock();
};

/// @}

#endif
