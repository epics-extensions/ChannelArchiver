#ifndef __GROUPINFO_H__
#define __GROUPINFO_H__

#include "ChannelInfo.h"

BEGIN_NAMESPACE_CHANARCH

//CLASS GroupInfo
// Several Channels,
// in the Engine identified by ChannelInfo,
// might form a group that can e.g. be disabled as a whole.
//
// This is double-linked:
// Each ChannelInfo indicates membership to several _groups
// so that a channel can quickly diable a group.
// Each GroupInfo knows all it's _members in order
// to disable them.
class GroupInfo
{
public:
	GroupInfo ();
	GroupInfo (const GroupInfo &);

	void setName (const stdString &name)			{ _name = name; }
	const stdString &getName () const				{ return _name; }

	// Disabling channels use the ID
	// to see if they disable this group:
	size_t getID ()	const							{ return _ID; }

	void addChannel (ChannelInfo *channel);

	const list<ChannelInfo *>&getChannels () const	{ return _members; }

	void disable (ChannelInfo *cause);
	void enable (ChannelInfo *cause);
	bool isEnabled () const							{ return _disabled <= 0; }

	// Info on connected channels out of getChannels.size():
	size_t getConnectedChannels () const			{ return _num_connected; }
	void incConnectedChannels ()					{ ++ _num_connected; }
	void decConnectedChannels ()					{ if (_num_connected > 0) --_num_connected; }

private:
	GroupInfo & operator = (const GroupInfo &); // not impl.

	static size_t	_next_ID;
	size_t			_ID;
	size_t			_num_connected;
	stdString		_name;
	list<ChannelInfo *>	_members;
	size_t			_disabled;	// disabled by how many channels?
};

END_NAMESPACE_CHANARCH

#endif //__GROUPINFO_H__
