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
#include <list>
#include <vector>

using namespace std;
BEGIN_NAMESPACE_CHANARCH

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
// <LI>The first valid line must be<BR>
//	   <PRE>master_version=1</PRE>
// <LI>All remaining lines list one archive name per line
// </UL>
// <H3>Example<H3>
// <PRE>
//	# ChannelArchiver master file
//	master_version=1
//	# First check the "fast" archive
//	/archives/fast/dir
//	# Then check the "main" archive
//	/archives/main/dir
//	# Then check Fred's "xyz" archive
//	/home/fred/xyzarchive/dir
// </PRE>
// <P>
// This type of archive is read-only!
// <P>
// For now, each individual archive is in the binary data format (CLASS BinArchive).
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
	MultiArchive (const stdString &master_file);

	//* All virtuals from CLASS ArchiveI are implemented,
	// except that the "write" routines fail for this read-only archive type.

	virtual ChannelIteratorI *newChannelIterator () const;
	virtual ValueIteratorI *newValueIterator () const;
	virtual ValueI *newValue (DbrType type, DbrCount count);
	virtual bool findFirstChannel (ChannelIteratorI *channel);
	virtual bool findChannelByName (const stdString &name, ChannelIteratorI *channel);
	virtual bool findChannelByPattern (const stdString &regular_expression, ChannelIteratorI *channel);
	virtual bool addChannel (const stdString &name, ChannelIteratorI *channel);

	// debugging only
	void log ();

	// To be used by MultiArchive intrinsics only:
	// -------------------------------------------
	bool getChannel (size_t channel_index, MultiChannelIterator &iterator) const; 
	const ChannelInfo & getChannelInfo (size_t channel_index) const; 

	bool getValueAfterTime (size_t channel_index, MultiChannelIterator &channel_iterator,
				const osiTime &time, MultiValueIterator &value_iterator) const;

private:
	bool parseMasterFile (const stdString &master_file);

	// Fill _channels from _archives
	bool investigateChannels ();
	bool findChannelInfo (const stdString &name, ChannelInfo **info);

	list<stdString>	_archives; // names of archives
	vector<ChannelInfo>	_channels; // info of channels, summarized over all archives
};

inline const ChannelInfo &MultiArchive::getChannelInfo (size_t channel_index) const
{	return _channels[channel_index];	}

END_NAMESPACE_CHANARCH

#endif
