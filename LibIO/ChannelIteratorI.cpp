#include "ArchiveI.h"

BEGIN_NAMESPACE_CHANARCH

ChannelIteratorI::~ChannelIteratorI ()
{}

ChannelIterator::ChannelIterator (const Archive &archive)
{	_ptr = archive.getI()->newChannelIterator ();	}

END_NAMESPACE_CHANARCH

