#include "Configuration.h"

Configuration::~Configuration ()
{ }

bool Configuration::save ()
{	return false; }

bool Configuration::saveEngine ()
{	return save ();	}

bool Configuration::saveChannel (const class ChannelInfo *channel)
{	return save ();	}

bool Configuration::saveGroup (const class GroupInfo *group)
{	return save ();	}

