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
	return 0;
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
	return false;
}

bool MultiArchive::findChannelByName (const stdString &name, ChannelIteratorI *channel)
{
	return false;
}

bool MultiArchive::findChannelByPattern (const stdString &regular_expression,
	ChannelIteratorI *channel)
{
	return false;
}

bool MultiArchive::addChannel (const stdString &name, ChannelIteratorI *channel)
{
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

void MultiArchive::fill_channels (const stdString &channel, void *arg)
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
	name_tree.traverse (fill_channels, this);

	return true;
}

void MultiArchive::log ()
{
	LOG_MSG ("MultiArchive:\n");
	list<stdString>::const_iterator i = _archives.begin();
	
	while (i != _archives.end())
	{
		LOG_MSG ("Archive file: " << *i << "\n");
		++i;
	}

	LOG_MSG ("Channels:\n");
	i = _channels.begin();
	while (i != _archives.end())
	{
		LOG_MSG (*i << "\n");
		++i;
	}
}


END_NAMESPACE_CHANARCH
