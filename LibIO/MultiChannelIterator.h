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
#include "MultiChannel.h"
#include "MultiArchive.h"

BEGIN_NAMESPACE_CHANARCH

class MultiValueIterator;

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
	// To be called by MultiArchive:
	void clear ();
	void position (size_t index, ArchiveI *archive, ChannelIteratorI *channel_iterator);

	// To be called by MultiChannel:
	const MultiArchive *getArchive ();
	const ChannelInfo &getChannelInfo () const;
	size_t getChannelIndex () const;

	// To be called by MultiValueIterator:
	bool getNextValue (MultiValueIterator &value_iterator);
	bool getPrevValue (MultiValueIterator &value_iterator);

private:
	bool _is_valid;
	const MultiArchive *_multi_archive;	// MultiArchive this iterator operates on
	size_t _channel_index;				// Index of channel in that archive
	ArchiveI *_base_archive;			// If valid, this is the actual archive...
	ChannelIteratorI *_base_channel_iterator; // and channeliterator for the current channel
	MultiChannel _channel;				// The current Channel (use only if _is_valid)
};

inline const MultiArchive *MultiChannelIterator::getArchive ()
{	return _multi_archive;	}

inline const ChannelInfo &MultiChannelIterator::getChannelInfo () const
{	return _multi_archive->getChannelInfo (_channel_index);	}

inline size_t MultiChannelIterator::getChannelIndex () const
{	return _channel_index;	}

END_NAMESPACE_CHANARCH

#endif //__MULTICHANNELITERATORI_H__
