#ifndef __CHANNELITERATORI_H__
#define __CHANNELITERATORI_H__

#include "ChannelI.h"

BEGIN_NAMESPACE_CHANARCH

//CLASS ChannelIteratorI
// Channel iterator interface to be implemented.
// The preferred API is CLASS ChannelIterator.
class ChannelIteratorI
{
public:
	virtual ~ChannelIteratorI ();

	//* Does Iterator hold valid channel ?
	virtual bool isValid () const = 0;

	//* Access to Channel
	virtual ChannelI *getChannel () = 0;

	//* Move to next channel
	virtual bool next () = 0;

};

//CLASS ChannelIterator
// Iterator for looping over channels,
// can be used similar to CLASS ChannelI pointer:
// <H2>Examples</H2>
// <UL>
// <LI>
// <PRE>
//	Archive archive (new BinArchive (archive_name));
//	ChannelIterator channel (archive);
//	
//	archive.findFirstChannel (channel);
//	while (channel)
//	{
//		cout << channel->getName() << endl;
//		++ channel;
//	}                             
// </PRE>
// </UL>
//
// (Implemented as "smart pointer" wrapper for CLASS ChannelIteratorI interface)
class ChannelIterator
{
public:
	//* Obtain new, empty ChannelIterator suitable for given Archive
	ChannelIterator (const class Archive &archive);

	ChannelIterator (ChannelIteratorI *iter);
	ChannelIterator ();

	~ChannelIterator ();

	void attach (ChannelIteratorI *iter);

	//* Does Iterator hold valid channel ?
	operator bool () const;

	//* Access to Channel
	ChannelI *operator -> ();
	ChannelI &operator * ();

	//* Move to next channel
	ChannelIterator &operator ++ ();

	// Direct access to interface
	ChannelIteratorI *getI ()				{	return _ptr; }
	const ChannelIteratorI *getI () const	{	return _ptr; }

private:
	ChannelIterator (const ChannelIterator &rhs); // not impl.
	ChannelIterator & operator = (const ChannelIterator &rhs); // not impl.

	ChannelIteratorI *_ptr;
};

inline ChannelIterator::ChannelIterator (ChannelIteratorI *iter)
{	_ptr = iter;	}

inline ChannelIterator::ChannelIterator ()
{	_ptr = 0;	}

inline ChannelIterator::~ChannelIterator ()
{	delete _ptr; }

inline ChannelIterator::operator bool () const
{	return _ptr->isValid ();	}

inline ChannelI *ChannelIterator::operator -> ()
{	return _ptr->getChannel ();	}

inline ChannelI &ChannelIterator::operator * ()
{	return *_ptr->getChannel ();	}

inline ChannelIterator &ChannelIterator::operator ++ ()
{	_ptr->next (); return *this; }
	
inline void ChannelIterator::attach (ChannelIteratorI *iter)
{
	delete _ptr;
	_ptr = iter;
}

END_NAMESPACE_CHANARCH

#endif //__CHANNELITERATORI_H__
