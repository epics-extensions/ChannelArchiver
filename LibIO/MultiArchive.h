// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __MULTI_ARCHIVEI_H__
#define __MULTI_ARCHIVEI_H__

#include "ArchiveI.h"

class MultiChannelIterator;
class MultiValueIterator;

//CLASS MultiArchive
// The MultiArchive class implements a CLASS ArchiveI interface
// for accessing more than one archive.
// <P>
// The MultiArchive is configured via a "master" archive file,
// an ASCII file with the following format:
//
// <UL>
// <LI>Comments (starting with a number-sign) and empty lines are ignored
// <LI>The very first line must be<BR>
//     <PRE>master_version=1</PRE>
//     There must be no comments preceding this line!
// <LI>All remaining lines list one archive name per line
// </UL>
// <H2>Example</H2>
// <PRE>
//  master_version=1
//  # First check the "fast" archive
//  /archives/fast/dir
//  # Then check the "main" archive
//  /archives/main/dir
//  # Then check Fred's "xyz" archive
//  /home/fred/xyzarchive/dir
// </PRE>
// <P>
// This type of archive is read-only!
// <P>
// For now, each individual archive is in the binary data format (CLASS BinArchive).
// Later, it might be necessary to specify the type together with the name
// for each archive. The master_version will then be incremented.
// <BR>
// If the "master" file is invalid, it is considered an ordinary BinArchive directory file,
// i.e. Tools based on the MultiArchive should work just like BinArchive-based Tools
// when operating on a single archive.
//
// <H2>Details</H2>
// No sophisticated merging technique is used.
//
// The MultiArchive will allow read access to
// the union of the individual channel sets.
//
// When combining archives with disjunct channel sets,
// a read request for a channel will yield data
// from the single archive that holds that channel.
//
// When channels are present in multiple archives,
// a read request for a given point in time will
// return values from the <I>first archive listed</I> in the master file
// that has values for that channel and point in time.
//
// Consequently, one should avoid archives with overlapping time ranges.
// If this cannot be avoided, the "most important" archive should be
// listed first.

class MultiArchive : public ArchiveI
{
public:
    //* Open a MultiArchive for the given master file
    MultiArchive(const stdString &master_file);

    //* All virtuals from CLASS ArchiveI are implemented,
    // except that the "write" routines fail for this read-only archive type.

    virtual ChannelIteratorI *newChannelIterator() const;
    virtual ValueIteratorI *newValueIterator() const;
    virtual ValueI *newValue(DbrType type, DbrCount count);
    virtual bool findFirstChannel(ChannelIteratorI *channel);
    virtual bool findChannelByName(const stdString &name, ChannelIteratorI *channel);
    virtual bool findChannelByPattern(const stdString &regular_expression, ChannelIteratorI *channel);
    virtual bool addChannel(const stdString &name, ChannelIteratorI *channel);

    // debugging only
    void log() const;

    // To be used by MultiArchive intrinsics only:
    // -------------------------------------------
    bool getChannel(size_t channel_index, MultiChannelIterator &iterator) const; 
    const ChannelIInfo & getChannelInfo(size_t channel_index) const; 

    // For given channel, set value_iterator to value at-or-after time.
    // For has_to_be_later = true, the archive must contain more values,
    // i.e. it won't position on the very last value that's stamped at "time" exactly
    //
    // For result=false, value_iterator could not be set.
    // These routines will not clear() the value_iterator
    // to allow stepping back when used from within next()/prev()!
    bool getValueAtOrAfterTime(size_t channel_index, MultiChannelIterator &channel_iterator,
                               const osiTime &time, bool has_to_be_later,
                               MultiValueIterator &value_iterator) const;
    bool getValueAtOrBeforeTime(size_t channel_index, MultiChannelIterator &channel_iterator,
                                const osiTime &time, bool has_to_be_earlier,
        MultiValueIterator &value_iterator) const;
    bool getValueNearTime(size_t channel_index, MultiChannelIterator &channel_iterator,
                          const osiTime &time, MultiValueIterator &value_iterator) const;

private:
    bool parseMasterFile(const stdString &master_file);

    // Fill _channels from _archives
    bool investigateChannels();
    bool findChannelInfo(const stdString &name, ChannelIInfo **info);

    stdList<stdString>      _archives; // names of archives
    stdVector<ChannelIInfo> _channels; // info of channels, summarized over all archives
};

inline const ChannelIInfo &MultiArchive::getChannelInfo(size_t channel_index) const
{   return _channels[channel_index];    }

#endif
