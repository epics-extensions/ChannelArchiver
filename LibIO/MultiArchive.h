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

using namespace std;
BEGIN_NAMESPACE_CHANARCH

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

class MultiArchive : public ArchiveI
{
public:
	//* Open a MultiArchive for the given master file
	MultiArchive (const stdString &master_file);

	virtual ~MultiArchive ();

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

private:
	bool parseMasterFile (const stdString &master_file);

	// Fill _channels from _archives
	bool listChannels ();
	static void fill_channels(const class stdString &, void *); // helper for this

	list<stdString>	_archives; // names of archives
	list<stdString>	_channels; // names of channels in all archives
};

END_NAMESPACE_CHANARCH

#endif //__ARCHIVEI_H__
