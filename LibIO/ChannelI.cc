// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "ChannelI.h"

ChannelI::~ChannelI()
{}

size_t ChannelI::lockBuffer(const ValueI &value, double period)
{	return 0; }
	
void ChannelI::addBuffer(const ValueI &value_arg, double period,
                         size_t value_count)
{}

// Call after adding all values to that buffer
void ChannelI::releaseBuffer()
{}


