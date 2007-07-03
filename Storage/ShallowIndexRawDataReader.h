// -*- c++ -*-

#ifndef __SHALLOW_INDEX_RAW_DATA_READER_H__
#define __SHALLOW_INDEX_RAW_DATA_READER_H__

// Tools
#include <ToolsConfig.h>
#include <AutoPtr.h>
// Storage
#include "DataReader.h"

/// \addtogroup Storage
/// @{

/** RawDataReader that understands 'shallow' indices.
 *  <p>
 *  As long as a 'shallow' index is hit, this reader keeps drilling down
 *  to the first 'full' index, and then uses a RawDataReader to get the
 *  samples.
 */
class ShallowIndexRawDataReader : public DataReader
{
public:
    ShallowIndexRawDataReader(Index &index);
    virtual ~ShallowIndexRawDataReader();
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
    /** Top-most index; where the search starts */
    Index &top_index;
    
    /** Channel found in top_index */
    AutoPtr<Index::Result> top_result;
      
    /** Current data block from top_result */
    AutoPtr<RTree::Datablock> top_datablock;
    
    /** The sub index.
     *  Used iteratively while following the chain of sub indices.
     *  In the end, this is the bottom-most index, the one
     *  that points to the actual data.
     */
    AutoPtr<Index> sub_index;

    /** Channel found in sub_index. */
    AutoPtr<Index::Result> sub_result;
    
    /** Data block from DataFile received via sub_result. */
    AutoPtr<RTree::Datablock> sub_datablock;
    
    /** The reader for the actual 'full' index that we might have
     *  found under the 'shallow' index.
     */
    AutoPtr<DataReader> reader;

    /** Find channel_name and start data,
     *  if necessary recursively going down,
     *  beginning with index/result/datablock.
     */
    const RawValue::Data *find(Index &index,
                               AutoPtr<Index::Result> &index_result,
                               AutoPtr<RTree::Datablock> &datablock,
                               const stdString &channel_name,
                               const epicsTime *start);
};

/// @}

#endif
