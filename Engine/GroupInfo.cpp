// GroupInfo.cpp

#include "GroupInfo.h"
#include "ArchiveChannel.h"

size_t GroupInfo::next_ID = 0;

GroupInfo::GroupInfo (const stdString &name)
{
    this->name = name;
	ID = next_ID++;
	num_connected = 0;
	disable_count = 0;
}

void GroupInfo::addChannel(ArchiveChannel *channel)
{
	// Is Channel already in group?
	stdList<ArchiveChannel *>::iterator i;
	for (i=members.begin(); i!=members.end(); ++i)
		if (*i == channel)
			return;
	members.push_back(channel);
}

// called by ArchiveChannel while channel is locked
void GroupInfo::disable(ArchiveChannel *cause, const epicsTime &when)
{
	LOG_MSG("'%s' disables group '%s'\n",
            cause->getName().c_str(), name.c_str());
	++disable_count;
	if (disable_count != 1) // Was already disabled?
		return;

	stdList<ArchiveChannel *>::iterator i;
	for (i=members.begin(); i!=members.end(); ++i)
		(*i)->disable(when);
}

// called by ArchiveChannel while channel is locked
void GroupInfo::enable(ArchiveChannel *cause, const epicsTime &when)
{
	LOG_MSG("'%s' enables group '%s'\n",
            cause->getName().c_str(), name.c_str());
	if (disable_count <= 0)
	{
		LOG_MSG("Group %d is not disabled, ERROR!\n", name.c_str());
		return;
	}
	--disable_count;
	if (disable_count > 0) // Still disabled?
		return;

	stdList<ArchiveChannel *>::iterator i;
	for (i=members.begin(); i!=members.end(); ++i)
		(*i)->enable(when);
}

#if 0
GroupInfo::GroupInfo (const GroupInfo &rhs)
{
	_ID = rhs._ID;
	_num_connected = rhs._num_connected;
	_name = rhs._name;
	_members = rhs._members;
	_disabled = rhs._disabled;
}
#endif
