#ifndef __BINCHANNELITERATOR_H__
#define __BINCHANNELITERATOR_H__

#include "ChannelIteratorI.h"
#include "DirectoryFile.h"
#include "RegularExpression.h"

BEGIN_NAMESPACE_CHANARCH

class BinArchive;
class DataHeaderIterator;

class BinChannelIterator : public ChannelIteratorI
{
public:
	BinChannelIterator ();
	BinChannelIterator & operator = (const BinChannelIterator &);
	~BinChannelIterator ();

	void attach (BinArchive *arch, const DirectoryFileIterator &dir, const char *regular_expression=0);

	// ChannelIterator interface
	virtual bool isValid () const;
	virtual ChannelI *getChannel ();
	virtual bool next ();

	BinArchive *getArchive ();

private:
	BinChannelIterator (const BinChannelIterator &rhs); //  not impl.
	void init ();
	void clear ();

	friend class BinChannel;

	BinArchive				*_archive;
	DirectoryFileIterator	_dir;
	RegularExpression		*_regex;
};

inline BinArchive *BinChannelIterator::getArchive ()
{	return _archive;	}

END_NAMESPACE_CHANARCH

#endif //__BINCHANNELITERATOR_H__

