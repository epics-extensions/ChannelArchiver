// GroupInfo.cpp

#include "GroupInfo.h"

size_t GroupInfo::_next_ID = 0;

GroupInfo::GroupInfo ()
{
	_disabled = 0;
	_ID = _next_ID++;
	_num_connected = 0;
}

GroupInfo::GroupInfo (const GroupInfo &rhs)
{
	_ID = rhs._ID;
	_num_connected = rhs._num_connected;
	_name = rhs._name;
	_members = rhs._members;
	_disabled = rhs._disabled;
}

// Add channel to this group.
// When added again, nothing will happen...
void GroupInfo::addChannel (ChannelInfo *channel)
{
	// Is Channel already in group?
	stdList<ChannelInfo *>::iterator i;
	for (i=_members.begin(); i!=_members.end(); ++i)
		if (*i == channel)
			return;
	_members.push_back (channel);
}

void GroupInfo::disable (ChannelInfo *cause)
{
	LOG_MSG (osiTime::getCurrent() << ", " << cause->getName ()
             << ": Disables group " << _name << "\n");
	++_disabled;
	if (_disabled != 1) // Was already disabled?
		return;

	stdList<ChannelInfo *>::iterator i;
	for (i=_members.begin(); i!=_members.end(); ++i)
		(*i)->disable (cause);
}

void GroupInfo::enable (ChannelInfo *cause)
{
	LOG_MSG (osiTime::getCurrent() << ", " << cause->getName ()
             << ": Enables group " << _name << "\n");
	if (_disabled <= 0)
	{
		LOG_MSG (osiTime::getCurrent()
                 << ": Group " << _name << " is not disabled, ERROR!\n");
		return;
	}
	--_disabled;
	if (_disabled > 0) // Still disabled?
		return;

	stdList<ChannelInfo *>::iterator i;
	for (i=_members.begin(); i!=_members.end(); ++i)
		(*i)->enable (cause);
}
