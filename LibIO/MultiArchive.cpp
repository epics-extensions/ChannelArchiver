// --------------------------------------------------------
// $Id$
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

#include "MultiArchive.h"
#include "MultiChannelIterator.h"
#include "MultiValueIterator.h"
#include "BinArchive.h"
#include "ArchiveException.h"
#include <ASCIIParser.h>
#include <BinaryTree.h>

BEGIN_NAMESPACE_CHANARCH


// Open a MultiArchive for the given master file
MultiArchive::MultiArchive (const stdString &master_file)
{
	parseMasterFile (master_file);
}

ChannelIteratorI *MultiArchive::newChannelIterator () const
{
	return new MultiChannelIterator (this);
}

ValueIteratorI *MultiArchive::newValueIterator () const
{
	return new MultiValueIterator ();
}

ValueI *MultiArchive::newValue (DbrType type, DbrCount count)
{
	return 0;
}

bool MultiArchive::findFirstChannel (ChannelIteratorI *channel)
{
	MultiChannelIterator *iterator = dynamic_cast<MultiChannelIterator *>(channel);
	return getChannel (0, *iterator);
}

bool MultiArchive::findChannelByName (const stdString &name, ChannelIteratorI *channel)
{
	MultiChannelIterator *iterator = dynamic_cast<MultiChannelIterator *>(channel);
	for (size_t i=0; i<_channels.size(); ++i)
	{
		if (_channels[i]._name == name)
			return getChannel (i, *iterator);
	}
	iterator->clear ();
	return false;
}

bool MultiArchive::findChannelByPattern (const stdString &regular_expression,
	ChannelIteratorI *channel)
{
	// TODO: Use RegularExpression on _channels
	return false;
}

bool MultiArchive::addChannel (const stdString &name, ChannelIteratorI *channel)
{
	throwDetailedArchiveException (Invalid, "Cannot write, MultiArchive is read-only");
	return false;
}

// Open Archive and ChannelIterator for channel described by ChannelInfo
// using the archive that has the oldest value
bool MultiArchive::getChannel (size_t channel_index, MultiChannelIterator &iterator) const
{
	if (channel_index >= _channels.size())
	{
		// Invalidate iterator
		iterator.clear ();
		return false;
	}

	const ChannelInfo &info = _channels[channel_index];
	list<stdString>::const_iterator archs = _archives.begin();
	for (/**/; archs != _archives.end(); ++archs)
	{
		Archive archive (new BinArchive (*archs));
		ChannelIterator channel (archive);
		if (archive.findChannelByName (info._name, channel))
		{
			// have "better" archive for this channel ?
			if (isValidTime (info._first_time)  &&
				channel->getFirstTime() > info._first_time)
				continue;

			iterator.position (channel_index, archive.getI(), channel.getI());
			archive.detach(); // Interfaces now ref'd by MultiChannelIterator
			channel.detach();
		}                    
	}

	return iterator.isValid ();
}

bool MultiArchive::getValueAfterTime (size_t channel_index, MultiChannelIterator &channel_iterator,
	const osiTime &time, MultiValueIterator &value_iterator) const
{
	if (channel_index >= _channels.size())
	{
		// Invalidate iterator
		value_iterator.clear ();
		return false;
	}

	const ChannelInfo &info = _channels[channel_index];
	list<stdString>::const_iterator archs = _archives.begin();
	for (/**/; archs != _archives.end(); ++archs)
	{
		Archive archive (new BinArchive (*archs));
		ChannelIterator channel (archive);
		if (archive.findChannelByName (info._name, channel))
		{
			ValueIterator value (archive);
			// Does this archive have values after "time"?
			if (! channel->getValueAfterTime (time, value))
				continue;

			channel_iterator.position (channel_index, archive.getI(), channel.getI());
			value_iterator.position (&channel_iterator, value.getI());

			value.detach ();	// Now ref'd by MultiValueIterator
			archive.detach();	// Now ref'd by MultiChannelIterator
			channel.detach();	// dito
			return value_iterator.isValid ();
		}                    
	}
	return false;
}

bool MultiArchive::parseMasterFile (const stdString &master_file)
{
	ASCIIParser	parser;

	if (! parser.open (master_file))
	{
		LOG_MSG ("Cannot open master file '" << master_file << "'\n");
		return false;
	}

	stdString parameter, value;
	if (parser.nextLine()						&&
		parser.getParameter (parameter, value)	&&
		parameter == "master_version"			&&
		value == "1")
	{
		while (parser.nextLine ())
			_archives.push_back (parser.getLine ());
	}
	else
	{
		LOG_MSG ("Expecting master_version=1 in '" << master_file
			<< "', line " << parser.getLineNo() << "\n");
		LOG_MSG ("Assuming '" << master_file << "' is a BinArchive\n");
		_archives.push_back (master_file);
	}

	investigateChannels ();

	log ();

	return true;
}

// Find ChannelInfo for given name,
// make info point to it.
// Result: true if existing one was found,
// false if new one was added.
bool MultiArchive::findChannelInfo (const stdString &name, ChannelInfo **info)
{
	// Find element we look for
	// or following element, >=, where we could insert
	vector<ChannelInfo>::iterator i = _channels.begin();
	for (/**/;  i != _channels.end(); ++i)
	{
		if (i->_name == name)	// found
		{
			*info = & *i;
			return true;
		}
		if (i->_name > name)	// past, insert here
			break;
	}

	ChannelInfo	new_info;
	new_info._name = name;
	*info = & *_channels.insert (i, new_info);

	return false;
}

// Fill _channels from _archives
bool MultiArchive::investigateChannels ()
{
	stdString name;
	ChannelInfo *info;

	_channels.clear ();

	list<stdString>::const_iterator archs = _archives.begin();
	for (/**/; archs != _archives.end(); ++archs)
	{
		Archive archive (new BinArchive (*archs));
		ChannelIterator channel (archive);
		archive.findFirstChannel (channel);
		while (channel)
		{
			name = channel->getName();
			if (findChannelInfo (name, &info))	// check bounds for existing channel
			{
				if (info->_first_time > channel->getFirstTime ())
					info->_first_time = channel->getFirstTime ();
				if (info->_last_time < channel->getLastTime ())
					info->_last_time = channel->getLastTime ();
			}
			else	// new entry:
			{
				info->_first_time = channel->getFirstTime ();
				info->_last_time = channel->getLastTime ();
			}
			++ channel;
		}         
	}

	return true;
}

void MultiArchive::log ()
{
	LOG_MSG ("MultiArchive:\n");
	list<stdString>::const_iterator archs = _archives.begin();
	
	while (archs != _archives.end())
	{
		LOG_MSG ("Archive file: " << *archs << "\n");
		++archs;
	}

	LOG_MSG ("Channels:\n");
	vector<ChannelInfo>::const_iterator chans = _channels.begin();
	while (chans != _channels.end())
	{
		LOG_MSG (chans->_name << "\n");
		++chans;
	}
}

END_NAMESPACE_CHANARCH
