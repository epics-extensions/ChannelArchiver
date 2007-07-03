#ifndef MASTERINDEX_H_
#define MASTERINDEX_H_

// Tools
#include <Index.h>

/** Helper for creating a full 'master' index:
 *  References every data block in all sub archives
 *  (except for 'chained' blocks).
 */
class MasterIndex
{
public:
    MasterIndex(const stdString &index_name, int RTreeM,
                const stdList<stdString> &subarchives, int verbosity)
        : index_name(index_name), RTreeM(RTreeM), subarchives(subarchives),
          verbosity(verbosity)
    {}
    
    virtual ~MasterIndex();

    void create();

protected:
    const stdString &index_name;
    int RTreeM;
    const stdList<stdString> &subarchives;
    int verbosity;
    void addSubindex(Index &index, const stdString &sub_name);
    virtual void addChannelInfo(const stdString &channel,
                                Index &index, Index::Result *dest_info,
                                Index &subindex, Index::Result *src_info);
};

/** Helper for creating a shallow 'master' index:
 *  References each channel and its time range in all sub-archives,
 *  but does not reference the actual data blocks.
 *  So this one is smaller than the full master index,
 *  but requires another lookup in the actual sub-archive's index.
 */
class ShallowIndex : public MasterIndex
{
public:
    ShallowIndex(const stdString &index_name, int RTreeM,
                const stdList<stdString> &subarchives, int verbosity)
        : MasterIndex(index_name, RTreeM, subarchives, verbosity)
    {}
protected:
    virtual void addChannelInfo(const stdString &channel,
                                Index &index, Index::Result *dest_info,
                                Index &subindex, Index::Result *src_info);
};

#endif /*MASTERINDEX_H_*/
