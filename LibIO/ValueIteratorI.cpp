#include "ArchiveI.h"

BEGIN_NAMESPACE_CHANARCH

ValueIteratorI::~ValueIteratorI ()
{}

ValueIterator::ValueIterator (const Archive &archive)
{	_ptr = archive.getI()->newValueIterator ();	}

END_NAMESPACE_CHANARCH

