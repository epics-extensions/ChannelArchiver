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
#include "BinArchive.h"
#include <ASCIIParser.h>
#include <BinaryTree.h>

BEGIN_NAMESPACE_CHANARCH


// Open a MultiArchive for the given master file
MultiArchive::MultiArchive (const stdString &master_file)
{
	parseMasterFile (master_file);
}

MultiArchive::~MultiArchive ()
{
}

ChannelIteratorI *MultiArchive::newChannelIterator () const
{
	return new MultiChannelIterator (this);
}

ValueIteratorI *MultiArchive::newValueIterator () const
{
	return 0;
}

ValueI *MultiArchive::newValue (DbrType type, DbrCount count)
{
	return 0;
}

bool MultiArchive::findFirstChannel (ChannelIteratorI *channel)
{
	MultiChannelIterator *iterator = dynamic_cast<MultiChannelIterator *>(channel);
	return getChannel (0, iterator);
}

bool MultiArchive::findChannelByName (const stdString &name, ChannelIteratorI *channel)
{
	MultiChannelIterator *iterator = dynamic_cast<MultiChannelIterator *>(channel);
	return getChannel (name, iterator);
}

bool MultiArchive::findChannelByPattern (const stdString &regular_expression,
	ChannelIteratorI *channel)
{
	// TODO: Use RegularExpression on _channels
	return false;
}

bool MultiArchive::addChannel (const stdString &name, ChannelIteratorI *channel)
{
	return false;
}

bool MultiArchive::getChannel (size_t channel_index, class MultiChannelIterator *iterator) const
{
	if (channel_index >= _channels.size())
	{
		// Invalidate iterator
		iterator->clear ();
		return false;
	}
	const stdString &channel_name = _channels[channel_index];

	LOG_MSG ("MultiArchive::getChannel (" << channel_index << ")\n");

	return getChannel (channel_name, iterator);
}

// Open Archive and ChannelIterator for channel_name,
// using the archive that has the oldest value
bool MultiArchive::getChannel (const stdString &channel_name, class MultiChannelIterator *iterator) const
{
	LOG_MSG ("MultiArchive::getChannel (" << channel_name << ")\n");

	osiTime start;
	list<stdString>::const_iterator archs = _archives.begin();
	for (/**/; archs != _archives.end(); ++archs)
	{
		Archive archive (new BinArchive (*archs));
		ChannelIterator channel (archive);
		if (archive.findChannelByName (channel_name, channel))
		{
			// already have "better" archive for this channel ?
			if (isValidTime (start)  &&  channel->getFirstTime() > start)
				continue;

			start = channel->getFirstTime();
			LOG_MSG ("found in " << *archs << " for " << start << "\n");
			iterator->position (archive.getI(), channel.getI());
			archive.detach(); // Interfaces now ref'd by MultiChannelIterator
			channel.detach();
		}                    
	}

	return iterator->isValid ();
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
	if (! parser.nextLine()							||
		! parser.getParameter (parameter, value)	||
		parameter != "master_version"				||
		value != "1")
	{
		LOG_MSG ("Expecting master_version=1 in '" << master_file
			<< "', line " << parser.getLineNo() << "\n");
		LOG_MSG ("Assuming '" << master_file << "' is a BinArchive\n");
		_archives.push_back (master_file);

		return true;
	}

	while (parser.nextLine ())
	{
		_archives.push_back (parser.getLine ());
	}

	listChannels ();

	log ();

	return true;
}

void MultiArchive::fill_channels_traverser (const stdString &channel, void *arg)
{
	MultiArchive *me = (MultiArchive *) arg;

	me->_channels.push_back (channel);
}

// Fill _channels from _archives
bool MultiArchive::listChannels ()
{
	BinaryTree<stdString> name_tree;

	list<stdString>::const_iterator archs = _archives.begin();
	
	while (archs != _archives.end())
	{
		Archive         archive (new BinArchive (*archs));
		ChannelIterator channel (archive);
		stdString name;

		archive.findFirstChannel (channel);
		while (channel)
		{
			name = channel->getName();
			if (! name_tree.find (name))
				name_tree.add (name);
			++ channel;
		}         
		
		++archs;
	}

	_channels.clear ();
	name_tree.traverse (fill_channels_traverser, this);

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
	vector<stdString>::const_iterator chans = _channels.begin();
	while (chans != _channels.end())
	{
		LOG_MSG (*chans << "\n");
		++chans;
	}
}

END_NAMESPACE_CHANARCH
