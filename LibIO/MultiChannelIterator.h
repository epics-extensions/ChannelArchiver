// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#ifndef __MULTICHANNELITERATORI_H__
#define __MULTICHANNELITERATORI_H__

#include "ChannelIteratorI.h"

BEGIN_NAMESPACE_CHANARCH

class MultiArchive;
class ArchiveI;
class ChannelIteratorI;

//CLASS MultiChannelIterator
// A CLASS ChannelIteratorI implementation for a CLASS MultiArchive
class MultiChannelIterator : public ChannelIteratorI
{
public:
	MultiChannelIterator (const MultiArchive *archive);
	virtual ~MultiChannelIterator ();

	virtual bool isValid () const;
	virtual ChannelI *getChannel ();
	virtual bool next ();

	// ---------------------------------------
	// To be called by MultiArchive only:
	void clear ();
	void position (ArchiveI *archive, ChannelIteratorI *channel_iterator);

private:
	bool _is_valid;
	const MultiArchive *_multi_archive;	// MultiArchive this iterator operates on
	size_t _channel_index;				// Index of channel in that archive
	ArchiveI *_base_archive;
	ChannelIteratorI *_base_channel_iterator;
};

END_NAMESPACE_CHANARCH

#endif //__MULTICHANNELITERATORI_H__
