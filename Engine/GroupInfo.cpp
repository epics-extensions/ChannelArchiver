// GroupInfo.cpp

// Tools
#include "MsgLogger.h"
// Engine
#include "Engine.h"
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

void GroupInfo::addChannel(Guard &engine_guard, Guard &channel_guard,
                           ArchiveChannel *channel)
{
    // Is Channel already in group?
    stdList<ArchiveChannel *>::iterator i;
    for (i=members.begin(); i!=members.end(); ++i)
        if (*i == channel)
            return;
    members.push_back(channel);
    if (disable_count > 0) // disable right away?
        channel->disable(engine_guard, channel_guard,
                         epicsTime::getCurrent());
}

// called by ArchiveChannel
void GroupInfo::disable(Guard &engine_guard,
                        ArchiveChannel *cause, const epicsTime &when)
{
    LOG_MSG("'%s' disables group '%s'\n",
            cause->getName().c_str(), name.c_str());
    ++disable_count;
    if (disable_count != 1) // Was already disabled?
        return;
    stdList<ArchiveChannel *>::iterator c;
    for (c=members.begin(); c!=members.end(); ++c)
    {
        Guard guard((*c)->mutex);
        (*c)->disable(engine_guard, guard, when);
    }
}

// called by ArchiveChannel
void GroupInfo::enable(Guard &engine_guard,
                       ArchiveChannel *cause, const epicsTime &when)
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
    stdList<ArchiveChannel *>::iterator c;
    for (c=members.begin(); c!=members.end(); ++c)
    {
        Guard guard((*c)->mutex);
        (*c)->enable(engine_guard, guard, when);
    }
}

